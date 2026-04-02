#include "auxiliarFunctions/functions.hpp"

void imprimeArray(std::ofstream& log, std::array<bool, 32> arr){
    for(int i = 0; i < 32; i++){log << arr[i];} // MSB primeiro, naturalmente
    log << "\n";
}

void imprimeArray8(std::ofstream& log, std::array<bool, 8> arr){
    for(int i = 0; i < 8; i++){log << arr[i];} // MSB primeiro, naturalmente
    log << "\n";
}

void imprimeRegistradores(std::ofstream& log,
                           Reg32_memory& MAR, Reg32_memory& MDR, Reg32_memory& PC,
                           Reg8& MBR, Reg32& SP, Reg32& LV, Reg32& CPP,
                           Reg32& TOS, Reg32& OPC, Reg32& H){

    log << "mar = "; imprimeArray(log, MAR.data);
    log << "mdr = "; imprimeArray(log, MDR.data);
    log << "pc = ";  imprimeArray(log, PC.data);
    log << "mbr = "; imprimeArray8(log, MBR.data);
    log << "sp = ";  imprimeArray(log, SP.data);
    log << "lv = ";  imprimeArray(log, LV.data);
    log << "cpp = "; imprimeArray(log, CPP.data);
    log << "tos = "; imprimeArray(log, TOS.data);
    log << "opc = "; imprimeArray(log, OPC.data);
    log << "h = ";   imprimeArray(log, H.data);
}

void imprimeMemoria(std::ofstream& log,
                    std::array<std::array<bool,32>, 16>& mem){
    log << "*******************************\n";
    for(int i = 0; i < 16; i++){
        for(int j = 0; j < 32; j++) log << mem[i][j];
        log << "\n";
    }
    log << "*******************************\n";
}

// Nomes dos registradores do barramento B
std::string nomeBarB(uint8_t codigo){
    switch(codigo){
        case 0: return "mdr";
        case 1: return "pc";
        case 2: return "mbr";
        case 3: return "mbru";
        case 4: return "sp";
        case 5: return "lv";
        case 6: return "cpp";
        case 7: return "tos";
        case 8: return "opc";
        default: return "none";
    }
}

// Nomes dos registradores habilitados no barramento C
std::string nomeBarC(uint16_t mascara){
    std::string resultado = "";
    // Bit: 8=H 7=OPC 6=TOS 5=CPP 4=LV 3=SP 2=PC 1=MDR 0=MAR
    if((mascara >> 8) & 1) resultado += "h, ";
    if((mascara >> 7) & 1) resultado += "opc, ";
    if((mascara >> 6) & 1) resultado += "tos, ";
    if((mascara >> 5) & 1) resultado += "cpp, ";
    if((mascara >> 4) & 1) resultado += "lv, ";
    if((mascara >> 3) & 1) resultado += "sp, ";
    if((mascara >> 2) & 1) resultado += "pc, ";
    if((mascara >> 1) & 1) resultado += "mdr, ";
    if((mascara >> 0) & 1) resultado += "mar, ";
    // Remove a vírgula e espaço finais
    if(!resultado.empty()) resultado = resultado.substr(0, resultado.size() - 2);
    return resultado;
}

Microinstrucao fromString(const std::string& s){
    Microinstrucao mi = {};
    for(int i = 0; i < 23; i++) mi[i] = (s[i] == '1');
    return mi;
}

TabelaMicro buildTabela(){
    TabelaMicro t;
    t["H_RECEBE_LV"]          = fromString("00010100" "100000000" "00" "0101");
    t["H_INCREMENTA"]         = fromString("00111001" "100000000" "00" "0000");
    t["MAR_RECEBE_H_RD"]      = fromString("00011000" "000000001" "01" "0000");
    t["MAR_SP_INCREMENTA_WR"] = fromString("00110101" "000001001" "10" "0100");
    t["TOS_RECEBE_MDR"]       = fromString("00010100" "001000000" "00" "0000");
    t["MAR_SP_INCREMENTA"]    = fromString("00110101" "000001001" "00" "0100");
    t["MDR_RECEBE_TOS_WR"]    = fromString("00010100" "000000010" "10" "0111");
    t["SP_MAR_INCREMENTA"]    = fromString("00110101" "000001001" "00" "0100");
    t["MDR_TOS_RECEBE_H_WR"]  = fromString("00011000" "001000010" "10" "0000");
    return t;
}

Programa traduzir(const std::vector<Token>& tokens, int& pos,
                  const TabelaMicro& tab){
    Programa micro;

    if(tokens[pos].tipo == Tipo::ILOAD){
        pos++;
        int x = std::stoi(tokens[pos].lexema);
        pos++;

        micro.push_back(tab.at("H_RECEBE_LV"));
        for(int i = 0; i < x; i++)
            micro.push_back(tab.at("H_INCREMENTA"));
        micro.push_back(tab.at("MAR_RECEBE_H_RD"));
        micro.push_back(tab.at("MAR_SP_INCREMENTA_WR"));
        micro.push_back(tab.at("TOS_RECEBE_MDR"));
    }
    else if(tokens[pos].tipo == Tipo::DUP){
        pos++;
        micro.push_back(tab.at("MAR_SP_INCREMENTA"));
        micro.push_back(tab.at("MDR_RECEBE_TOS_WR"));
    }
    else if(tokens[pos].tipo == Tipo::BIPUSH){
        pos++;
        std::string byteArg = tokens[pos].lexema;
        pos++;

        while(byteArg.size() < 8) byteArg = "0" + byteArg;
        byteArg = byteArg.substr(byteArg.size() - 8, 8);

        // Caso especial: byte nos 8 primeiros bits, MEM=11
        Microinstrucao fetchEspecial = {};
        for(int i = 0; i < 8;  i++) fetchEspecial[i]    = (byteArg[i] == '1');
        for(int i = 8; i < 17; i++) fetchEspecial[i]    = false; // C=0
        fetchEspecial[17] = true;  // WRITE
        fetchEspecial[18] = true;  // READ  → caso especial
        for(int i = 19; i < 23; i++) fetchEspecial[i]   = false; // B=0

        micro.push_back(tab.at("SP_MAR_INCREMENTA"));
        micro.push_back(fetchEspecial);
        micro.push_back(tab.at("MDR_TOS_RECEBE_H_WR"));
    }
    else {
        std::cerr << "[TRADUTOR Error] Token inesperado: "
                  << tokens[pos].lexema << "\n";
        pos++;
    }

    return micro;
}
