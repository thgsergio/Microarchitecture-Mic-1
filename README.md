# Microarchitecture-Mic-1

Simulador da microarquitetura Mic-1 implementado em C++ como projeto avaliativo da disciplina **Arquitetura de Computadores** (UFPB-CI).

O simulador lê instruções da ISA da IJVM (`BIPUSH`, `DUP`, `ILOAD`), traduz cada uma para uma sequência de microinstruções de 23 bits e as executa ciclo a ciclo, registrando o estado dos registradores e da memória em um log.

---

## Sumário

- [Estrutura do projeto](#estrutura-do-projeto)
- [Conceitos implementados](#conceitos-implementados)
  - [Registradores](#registradores)
  - [ULA](#ula)
  - [Deslocador](#deslocador)
  - [Barramentos e caminho de dados](#barramentos-e-caminho-de-dados)
  - [Microinstruções de 23 bits](#microinstruções-de-23-bits)
  - [Instruções IJVM](#instruções-ijvm)
- [Arquivos de entrada](#arquivos-de-entrada)
- [Compilação e execução](#compilação-e-execução)
- [Formato da saída](#formato-da-saída)

---

## Estrutura do projeto

```
Microarchitecture-Mic-1/ 
├── build/                             # Gerado pelo CMake
|   |
├── resultados/
│   └── log_execucao.txt               # Saída da simulação
|   |
├── src/
│   ├── Mic1.cpp                       # Ponto de entrada e loop principal
|   |
│   ├── ula/        
|   |   └── ULA.hpp
|   |   └── ULA.cpp
|   |
│   ├── reg/        
|   |   └── register.hpp
|   |   └── register.cpp
|   |
│   ├── lexer/      
|   |   └── lexer.hpp
|   |   └── lexer.cpp
|   |
│   ├── auxiliarFunctions/  
|   |   └── functions.hpp
|   |   └── functions.cpp
|   |
│   └── memory/
│   |   └── registradores.txt          # Estado inicial dos registradores
│   |   └── memory_Data.txt            # Memória de dados (16 × 32 bits)
│   |   └── instrucoes.txt             # Programa IJVM a executar
|   |
├── tests/
|   |
└── CMakeLists.txt
```

---

## Conceitos implementados

### Registradores

Dez registradores organizados em uma hierarquia de classes:

```
Reg  (base abstrata)
 ├── Reg32              → 32 bits: H, OPC, TOS, CPP, LV, SP, PC, MDR, MAR
 │    └── Reg32_memory  → Reg32 com acesso a arquivo .txt (MAR, MDR, PC)
 └── Reg8               → 8 bits: MBR
```

Cada registrador expõe `transf(bar)` para receber do barramento C e `recebe()` para enviar ao barramento B. O MBR tem ainda `recebeU()`, que faz extensão de zeros em vez de extensão de sinal (modo MBRU).

### ULA

A ULA recebe um **controle de 8 bits** no formato:

```
[ SLL8 | SRA1 | F0 | F1 | ENA | ENB | INVA | INC ]
   0      1     2    3    4     5     6      7
```

`F0` e `F1` selecionam o grupo de operação via `switch-case` aninhado. `ENA`, `ENB`, `INVA` e `INC` controlam as entradas, permitindo operações como `A+B`, `B+1`, `¬A`, `B−A`, entre outras. As somas são feitas bit a bit com carry propagado pelas funções `add1()` e `addXY()`.

A saída é uma struct com o resultado `s[32]`, o `carry_out` e as flags `N` (negativo) e `Z` (zero).

### Deslocador

Aplicado **após** a ULA, antes de o resultado ir ao barramento C:

- **SLL8** — desloca 8 bits à esquerda (lógico, preenche com 0)
- **SRA1** — desloca 1 bit à direita (aritmético, preserva o sinal)

Os dois nunca estão ativos ao mesmo tempo. `N` e `Z` são recalculados sobre o resultado final deslocado.

### Barramentos e caminho de dados

A entrada **A** da ULA sempre vem de **H**. A entrada **B** é selecionada pelo **decodificador** (4 bits, barramento B), que mapeia um código para um dos nove registradores:

| Código | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
|--------|---|---|---|---|---|---|---|---|---|
| Reg    | MDR | PC | MBR | MBRU | SP | LV | CPP | TOS | OPC |

A saída do deslocador vai ao **seletor** (9 bits, barramento C), que escreve o resultado em todos os registradores com bit ativo:

```
Bit:  8    7    6    5    4   3   2    1    0
Reg:  H   OPC  TOS  CPP  LV  SP  PC  MDR  MAR
```

A ordem de execução por ciclo é sempre: **ULA → deslocador → barramento C → memória**.

### Microinstruções de 23 bits

```
[ ULA 8b ][ C-bus 9b ][ Mem 2b ][ B-bus 4b ]
```

Os 2 bits de memória controlam READ e WRITE. Quando ambos estão em 1 ao mesmo tempo, é o **caso especial do BIPUSH**: os 8 bits do campo ULA são o próprio byte do argumento, carregado diretamente em MBR e H (sem passar pela ULA).

### Instruções IJVM

| Instrução | Microinstruções geradas |
|-----------|------------------------|
| `BIPUSH byte` | `SP=MAR=SP+1` → fetch especial (MBR←byte, H←byte) → `MDR=TOS=H; wr` |
| `DUP` | `MAR=SP=SP+1` → `MDR=TOS; wr` |
| `ILOAD x` | `H=LV` → `H=H+1` (×x) → `MAR=H; rd` → `MAR=SP=SP+1; wr` → `TOS=MDR` |

---

## Arquivos de entrada

**`registradores.txt`** — um registrador por linha:
```
sp  = 00000000000000000000000000000110
lv  = 00000000000000000000000000000011
tos = 00000000000000000000000000001000
mbr = 00000000
...
```

**`memory_Data.txt`** — 16 linhas de 32 bits cada:
```
00000000000000000000000000000000
00000000000000000000000000000001
...
```

**`instrucoes.txt`** — instruções IJVM, uma por linha (argumento do `BIPUSH` em binário, do `ILOAD` em decimal):
```
BIPUSH 10101010
DUP
ILOAD 2
```

---

## Compilação e execução

**Requisitos:** CMake ≥ 3.10, compilador C++17 (g++, Clang ou MSVC).

```bash
# Clonar e entrar no repositório
git clone https://github.com/thgsergio/Microarchitecture-Mic-1.git
cd Microarchitecture-Mic-1

# Configurar e compilar
mkdir build && cd build
cmake ..
cmake --build .

# Executar a partir da raiz do projeto
cd ..
./build/mic1
```

> O executável deve ser rodado da **raiz do projeto**, pois os caminhos para `src/memory/` e `resultados/` são relativos a ela.

A saída é escrita em `resultados/log_execucao.txt`.

---

## Formato da saída

```
============================================================
Initial memory state / Initial register state
============================================================
------------------------------------------------------------
Instruction: BIPUSH 10101010
------------------------------------------------------------
Cycle 1
ir = 00110101 000001001 00 0100
b = sp
c = sp, mar

> Registers before instruction
mar = 00000000000000000000000000000110
...

> Registers after instruction
mar = 00000000000000000000000000000111
...

> Memory after instruction
[linha 0]  00000000000000000000000000000000
...
============================================================
Cycle N
No more lines, EOP.
```

Cada ciclo registra a microinstrução executada, o registrador do barramento B, os registradores escritos pelo barramento C, e o estado completo dos registradores e da memória antes e depois da execução.

---

Projeto desenvolvido para a disciplina de Arquitetura de Computadores — UFPB, 2026.
Orientador — Prof. Augusto de Holanda B. M. Tavares.
Alunos — João Pedro, Luís Eduardo, Maria Vitória e Thiago Sérgio. 
