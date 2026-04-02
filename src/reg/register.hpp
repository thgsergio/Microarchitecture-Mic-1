#ifndef REGISTER_HPP
#define REGISTER_HPP
/**
 * Módulo: TAD Registrador
 * 
 * Objetivos:
 *  -> Definir os atributos, métodos e funções gerais dos registradores de acordo
 *  com os textos (slides e livro);
 *  -> Definir um inicializador geral para o registrador;
 * 
 * Observações:
 *  -> O registrador DEVE possuir o modo de Leitura e Escrita;
 *  -> O registrador DEVE converter o resultado de string para booleano (os que acessam a memória)
 *  -> Retorno: DEVE retornar ou um bool array[6] (o que conecta com a ULA)
 * 
 * Com os objetivos concluídos, será possível simular um registrador de N bits que
 * será utilizado no desenvolvimento da ULA.
*/

#include <iostream>
#include <fstream>
#include <vector>
#include <array> //Esta biblioteca vai ajudar.
#include <stdint.h>

/*O que um registrador deve possuir em geral? No caso do projeto, é claro*/
/**
 *  -> Nome;
 *  -> Vetor booleano (N bits);
 *  -> Duas variáveis (Usadas para definir as operações - leitura ou escrita);
 *  -> Forma de acessar o barramento B;
 *  -> Forma do barramento C acessar o registrador
 *  -> Forma de acessar a memória (em alguns casos).
 */

/* Ideia geral de uso dos Registradores (de 8 e 32 bits): */
/* 
 *  - Em relação aos atributos, as suas descrições estão acompanhadas de suas respectivas declarações
 *   abaixo no código.
 * - As funções transf() e recebe(), como ideia inicial, devem respectivamente receber como parâmetro
 *   o local para onde deve ser transferido (ex.: entrada A da ULA) e o local de onde o dado será 
 *   recebido (ex.: saída S da ULA)  
 * - Os barramentos serão considerados como ABSTRAÇÕES, uma vez que as funções anteriores já receberão
 *   os locais de origem e destino dos dados
 *
*/

/*Aqui está uma estrutura para um registrador geral*/
/**
 * Note o seguinte: as entradas (input 1 e 2) não necessariamente existem para todos os registradores, vide o  registra-
 * dor H (holder register). Nestes casos onde não há uma das entradas, vamos inicializar este registrador com o valor 
 * da porta não existente como FALSE, i.e, supondo o H, o input 2 seria FALSE, podendo só o input_1 variar.
 */
class Reg{
    protected:
        const std::string nome;    //Nome do registrador
        bool input_1;              //Entrada 1 ESQUERDA => Na imagem do caminho de dados, este imput é o do lado esquerdo
        bool input_2;              // Entrada 2 DIREITA  => Na imagem do caminho de dados, este imput é o do lado direito

    public:
        virtual bool transf(std::array<bool, 32>& bar) = 0;                //Transfere os dados para o barramento B
        virtual std::array<bool, 32> recebe() = 0;   
};

/* Classe Reg32, contém todos os parâmetros da classe Reg, mas com alterações na implementação das funções transf() e recebe()*/
class Reg32 : public Reg{
    public:
    /*Atributos*/
        std::array<bool, 32> data; //Informação do registrador                             //Os dados do barramento C são passados para o registrador
        
    /*Métodos*/    
        bool transf(std::array<bool, 32>& bar) override; // Transfere os dados para o barramento B
        std::array<bool, 32> recebe() override;
};

/*Esta classe é responsável pela interação com a memória (relacionados ao registradores de 32 bits, é claro)*/
class Reg32_memory : public Reg32 {  // ← adicionar public
    public:
        std::array<bool, 32> leituraMemory(std::string arquivo, uint32_t endereco); // Realiza a leitura DA memória (arquivo txt)
        bool escritaMemory(std::string arquivo, uint32_t endereco);                 // Realiza a escrita NA memória (arquivo txt)
};

/*Está classe é mais "tranquila", uma vez que só há o MBR com 8 bits*/
//Está classe tem interação com a memória
class Reg8 : public Reg{ //Toda informação do Reg32 pode ser aplicado aqui..
    public:
    /*Atrbutos*/
        std::array<bool, 8> data;

    /*Métodos*/
        /*Por que 32 bits? Simplesmente Extensão de sinal no barramento B*/
        bool transf(std::array<bool, 32>& bar) override;
        std::array<bool, 32> recebe() override;
        std::array<bool, 32> recebeU();
};

// Decodificador
std::array<bool, 32> decodificador(uint8_t entrada,
                                   Reg32 &OPC, Reg32 &TOS, Reg32 &CPP,
                                   Reg32 &LV, Reg32 &SP, Reg8 &MBR,
                                   Reg32 &PC, Reg32 &MDR);
// Seletor
// Convenção do enunciado:
// Bit: 8   7    6    5   4   3   2   1   0
// Reg: H  OPC  TOS  CPP  LV  SP  PC  MDR MAR
void seletor(uint16_t entrada, std::array<bool, 32> sd,
             Reg32& H,   Reg32& OPC, Reg32& TOS, Reg32& CPP,
             Reg32& LV,  Reg32& SP,  Reg32& PC,  Reg32& MDR,
             Reg32& MAR);

#endif // REGISTER_HPP