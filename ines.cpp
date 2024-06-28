#include <fstream>
#include "ines.h"
INES read_rom(const char *s){
    INES ines{};
    std::ifstream ifs(s,std::ios_base::in|std::ios_base::binary);
    if(!ifs){
        throw "file not found.";
    }
    ifs.read((char*)ines.header,16);
    ines.prg_size=ines.header[4];
    ines.chr_size=ines.header[5];
    ines.flag=ines.header[6];
    ifs.read((char*)ines.prg,16384*ines.prg_size);
    ifs.read((char*)ines.chr,8192*ines.chr_size);
    return ines;
}
