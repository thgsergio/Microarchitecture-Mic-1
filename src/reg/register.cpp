/**
 * Módulo: Implementação do registrador
 * 
 * Objetivo:
 *  -> Com as funções definidas em register.hpp
*/

#include "register.hpp"

/* ============ Reg32 ============ */
bool Reg32::transf(std::array<bool, 32>& bar){
    data = bar;
    return true;
}

std::array<bool, 32> Reg32::recebe(){
    return data; // Simplesmente expõe o valor atual ao barramento B
}

/* ============ Reg32_memory ============ */
std::array<bool, 32> Reg32_memory::leituraMemory(std::string arquivo, uint32_t endereco){
    std::ifstream file(arquivo);
    std::string linha;
    uint32_t linhaAtual = 0;

    while (std::getline(file, linha)){
        if (linhaAtual == endereco)        {
            // Converte a string "01001101..." para array<bool,32>
            for (int i = 0; i < 32; i++)
                data[i] = (linha[i] == '1');
            return data;
        }
        linhaAtual++;
    }
    // Endereço não encontrado — retorna zeros
    data.fill(false);
    return data;
}

bool Reg32_memory::escritaMemory(std::string arquivo, uint32_t endereco){
    // 1. Lê todas as linhas
    std::ifstream fileIn(arquivo);
    std::vector<std::string> linhas;
    std::string linha;

    while (std::getline(fileIn, linha)){
        linhas.push_back(linha);
    }
    fileIn.close();

    // 2. Verifica se o endereço é válido
    if (endereco >= linhas.size())
        return false;

    // 3. Converte data para string e substitui a linha
    std::string novaLinha = "";
    for (int i = 0; i < 32; i++){
        novaLinha += (data[i] ? '1' : '0');
    }
    linhas[endereco] = novaLinha;

    // 4. Reescreve o arquivo
    std::ofstream fileOut(arquivo);
    for (const auto& l : linhas){
        fileOut << l << "\n";
    }
    return true;
}

/* ============ Reg8 ============ */
bool Reg8::transf(std::array<bool, 32>& bar){
    // Pega apenas os 8 bits menos significativos do barramento
    for (int i = 0; i < 8; i++){
        data[i] = bar[24 + i];
    }
    return true;
}

std::array<bool, 32> Reg8::recebe(){
    std::array<bool, 32> extendido;
    bool bit_sinal = data[0]; // MSB do valor de 8 bits

    for (int i = 0; i < 24; i++){
        extendido[i] = bit_sinal; // Preenche com o bit de sinal
    }
    for (int i = 0; i < 8; i++){
        extendido[24 + i] = data[i]; // Copia os 8 bits originais
    }
    return extendido;
}

std::array<bool, 32> Reg8::recebeU(){
    std::array<bool, 32> extendido = {};

    for (int i = 0; i < 8; i++){
        extendido[24 + i] = data[i]; // Copia os 8 bits originais
    }
    return extendido;
}

/* ============ Decodificador ============ */
std::array<bool, 32> decodificador(uint8_t entrada, 
                                    Reg32& OPC, Reg32& TOS, Reg32& CPP,
                                    Reg32& LV,  Reg32& SP,  Reg8&  MBR,
                                    Reg32& PC,  Reg32& MDR) {
    switch (entrada) {
        case 0: return MDR.recebe();
        case 1: return PC.recebe();
        case 2: return MBR.recebe();    // Extensão de sinal
        case 3: return MBR.recebeU();   // Extensão com zeros (MBRU)
        case 4: return SP.recebe();
        case 5: return LV.recebe();
        case 6: return CPP.recebe();
        case 7: return TOS.recebe();
        case 8: return OPC.recebe();
        default:
            std::array<bool, 32> vazio = {};
            return vazio;
    }
}

/* ============ Seletor ============ */
void seletor(uint16_t entrada, std::array<bool, 32> sd,
             Reg32& H,   Reg32& OPC, Reg32& TOS, Reg32& CPP,
             Reg32& LV,  Reg32& SP,  Reg32& PC,  Reg32& MDR,
             Reg32& MAR) {

    if ((entrada >> 8) & 1) H.transf(sd);
    if ((entrada >> 7) & 1) OPC.transf(sd);
    if ((entrada >> 6) & 1) TOS.transf(sd);
    if ((entrada >> 5) & 1) CPP.transf(sd);
    if ((entrada >> 4) & 1) LV.transf(sd);
    if ((entrada >> 3) & 1) SP.transf(sd);
    if ((entrada >> 2) & 1) PC.transf(sd);
    if ((entrada >> 1) & 1) MDR.transf(sd);
    if ((entrada >> 0) & 1) MAR.transf(sd);
}
