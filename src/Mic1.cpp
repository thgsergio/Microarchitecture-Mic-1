/**
 * Módulo FINAL: Implementação COMPLETA da microarquitetura MIC-1
 * 
 *  Objetivo:
 *  -> Unificar TODOS os arquivos;
 *  -> Implementar a MIC-1 de forma coerente e consisa.
*/

#include <iostream>
#include <fstream>
#include <string>
#include <array>

#include "ula/ULA.hpp"
#include "reg/register.hpp"
#include "lexer/lexer.hpp"
#include "auxiliarFunctions/functions.hpp"


void executar(const Microinstrucao& mi, int ciclo,
              ULA& ula,
              std::array<std::array<bool,32>,16>& memoria,
              Reg32_memory& MAR, Reg32_memory& MDR, Reg32_memory& PC,
              Reg8& MBR,
              Reg32& SP, Reg32& LV, Reg32& CPP,
              Reg32& TOS, Reg32& OPC, Reg32& H,
              std::ofstream& log)
{
    // Parse da Microinstrucao — direto dos bits booleanos
    for(int i = 0; i < 8; i++)
        ula.co.control[i] = mi[i];
 
    uint16_t mascaraC = 0;
    for(int i = 8; i <= 16; i++)
        mascaraC = (mascaraC << 1) | mi[i];
 
    bool bitWRITE = mi[17];
    bool bitREAD  = mi[18];
 
    uint8_t codigoB = 0;
    for(int i = 19; i <= 22; i++)
        codigoB = (codigoB << 1) | mi[i];
 
    // Reconstrói string do IR apenas para exibição no log
    std::string irStr(23, '0');
    for(int i = 0; i < 23; i++) irStr[i] = mi[i] ? '1' : '0';
 
    // Cabeçalho do ciclo
    log << "Cycle " << ciclo << "\n";
    log << "ir = "
        << irStr.substr(0,  8) << " "
        << irStr.substr(8,  9) << " "
        << irStr.substr(17, 2) << " "
        << irStr.substr(19, 4) << "\n";
    log << "b = " << nomeBarB(codigoB) << "\n";
    log << "c = " << nomeBarC(mascaraC) << "\n";
    log << "\n";
 
    // Estado antes
    log << "> Registers before instruction\n";
    log << "*******************************\n";
    imprimeRegistradores(log, MAR, MDR, PC, MBR, SP, LV, CPP, TOS, OPC, H);
 
    // -------------------------------------------------------
    // Caso especial BIPUSH: WRITE e READ ambos em 1
    // Os 8 primeiros bits são o byte -> MBR e H (zero-extended)
    // -------------------------------------------------------
    if(bitWRITE && bitREAD){
        std::array<bool,8> byteVal = {};
        for(int i = 0; i < 8; i++){byteVal[i] = mi[i];}
        MBR.data = byteVal;
 
        // Zero-extension para 32 bits: byte ocupa os 8 bits menos significativos
        H.data = {};
        for(int i = 0; i < 8; i++) H.data[24 + i] = byteVal[i];
    }
    else {
        // 1. Entradas da ULA
        ula.in.A = H.recebe();
        ula.in.B = decodificador(codigoB, OPC, TOS, CPP, LV, SP, MBR, PC, MDR);
 
        // 2. Executa ULA e deslocador
        ULA_output out = ula.output();
        ula.deslocador(out);
 
        // 3. Escreve barramento C
        seletor(mascaraC, out.s, H, OPC, TOS, CPP, LV, SP, PC, MDR, MAR);
 
        // 4. Operação de memória — SEMPRE após barramento C
        if(bitWRITE){
            uint32_t endereco = 0;
            for(int i = 0; i < 32; i++) endereco = (endereco << 1) | MAR.data[i];
            if(endereco < 16) memoria[endereco] = MDR.data;
        }
        else if(bitREAD){
            uint32_t endereco = 0;
            for(int i = 0; i < 32; i++) endereco = (endereco << 1) | MAR.data[i];
            if(endereco < 16) MDR.data = memoria[endereco];
        }
    }
 
    // Estado depois
    log << "\n";
    log << "> Registers after instruction\n";
    log << "*******************************\n";
    imprimeRegistradores(log, MAR, MDR, PC, MBR, SP, LV, CPP, TOS, OPC, H);
 
    log << "\n";
    log << "> Memory after instruction\n";
    imprimeMemoria(log, memoria);
    log << "============================================================\n";
}


int main(){
    // Abertura de arquivos
    std::ifstream arqDados("src/memory/memory_Data.txt");
    //std::ifstream arqInstrucoes("scr/memory/instrucoes.txt");
    std::ofstream log("resultados/log_execucao.txt");
 
    if(!arqDados.is_open())      { std::cerr << "[FILE Error] memory_Data.txt\n";   return 1; }
    //if(!arqInstrucoes.is_open()) { std::cerr << "[FILE Error] instrucoes.txt\n";    return 1; }
    if(!log.is_open())           { std::cerr << "[FILE Error] log_execucao.txt\n";  return 1; }
 
    // Instâncias
    ULA           ula = {};
    Reg8          MBR = {};
    Reg32_memory  MAR = {}, MDR = {}, PC  = {};
    Reg32         SP  = {}, LV  = {}, CPP = {};
    Reg32         TOS = {}, OPC = {}, H   = {};
 
    // Memória de dados — 16 palavras de 32 bits
    std::array<std::array<bool,32>,16> memoria = {};
 
    // Carrega memória de dados
    {
        std::string linhaM;
        int idx = 0;
        while(std::getline(arqDados, linhaM) && idx < 16){
            if(!linhaM.empty() && linhaM.back() == '\r') linhaM.pop_back();
            if(linhaM.empty()) continue;
            for(int j = 0; j < 32; j++)
                memoria[idx][j] = (linhaM[j] == '1');
            idx++;
        }
    }
 
    // Carrega registradores iniciais
    std::string conteudo = getExpression("src/memory/registradores.txt");
    std::vector<Token> tokensReg = tokenize(conteudo);
 
    for(int i = 0; i < (int)tokensReg.size() - 1; ){
        if(tokensReg[i].tipo == Tipo::EOF_TOKEN) break;
        Token regTok  = tokensReg[i];
        Token attrTok = tokensReg[i+1];
        Token valTok  = tokensReg[i+2];
        if(attrTok.tipo != Tipo::Attr || valTok.tipo != Tipo::Numero){
            std::cerr << "[PARSE Error] Formato inválido no arquivo de registradores\n";
            return 1;
        }
        std::string valor = valTok.lexema;
        auto preenche32 = [&](Reg32& r){ for(int j=0;j<32;j++) r.data[j]=(valor[j]=='1'); };
        auto preenche8  = [&](Reg8&  r){ for(int j=0;j<8; j++) r.data[j]=(valor[j]=='1'); };
        switch(regTok.tipo){
            case Tipo::R_MAR: preenche32(MAR); break;
            case Tipo::R_MDR: preenche32(MDR); break;
            case Tipo::R_PC:  preenche32(PC);  break;
            case Tipo::R_MBR: preenche8(MBR);  break;
            case Tipo::R_SP:  preenche32(SP);  break;
            case Tipo::R_LV:  preenche32(LV);  break;
            case Tipo::R_CPP: preenche32(CPP); break;
            case Tipo::R_TOS: preenche32(TOS); break;
            case Tipo::R_OPC: preenche32(OPC); break;
            case Tipo::R_H:   preenche32(H);   break;
            default:
                std::cerr << "[PARSE Error] Token inesperado: " << regTok.lexema << "\n";
                return 1;
        }
        i += 3;
    }
 
    // Cabeçalho inicial
    log << "============================================================\n";
    log << "Initial memory state\n";
    imprimeMemoria(log, memoria);
    log << "Initial register state\n";
    imprimeRegistradores(log, MAR, MDR, PC, MBR, SP, LV, CPP, TOS, OPC, H);
    log << "============================================================\n";
    log << "Start of Program\n";
    log << "============================================================\n";
 
    // Inicializa tabela de microinstruções
    TabelaMicro tabela = buildTabela();
 
    // Tokeniza instrucoes.txt
    std::string conteudoInstr = getExpression("src/memory/instrucoes.txt");
    std::vector<Token> tokensInstr = tokenize(conteudoInstr);
 
    int pos   = 0;
    int ciclo = 0;
 
    // Loop principal: traduz cada instrução IJVM e executa suas microinstruções
    while(tokensInstr[pos].tipo != Tipo::EOF_TOKEN){
 
        // Label da instrução IJVM no log
        log << "------------------------------------------------------------\n";
        log << "Instruction: " << tokensInstr[pos].lexema;
        if(tokensInstr[pos].tipo == Tipo::ILOAD ||
           tokensInstr[pos].tipo == Tipo::BIPUSH)
            log << " " << tokensInstr[pos+1].lexema;
        log << "\n";
        log << "------------------------------------------------------------\n";
 
        // Traduz instrução IJVM -> microinstruções e executa cada uma
        Programa micros = traduzir(tokensInstr, pos, tabela);
        for(const Microinstrucao& mi : micros){
            ciclo++;
            executar(mi, ciclo, ula, memoria,
                     MAR, MDR, PC, MBR, SP, LV, CPP, TOS, OPC, H, log);
        }
    }
 
    // EOP
    ciclo++;
    log << "Cycle " << ciclo << "\n";
    log << "No more lines, EOP.\n";
 
    arqDados.close();
    //arqInstrucoes.close();
    log.close();
    return 0;
}