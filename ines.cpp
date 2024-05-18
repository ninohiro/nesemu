#include <cstdlib>
#include <vector>
#include <string>
#include <fstream>
#include <iterator>
#include "ines.h"
INES read_rom(const std::string &s){
    INES ines{};
    std::basic_ifstream<char> ifs(s,std::ios_base::in|std::ios_base::binary);
    if(!ifs){
        return ines;
    }
    std::istreambuf_iterator<char> it(ifs);
    std::istreambuf_iterator<char> last;
    std::vector<char> rom(it, last);
    ines.prg_size=(unsigned char)rom[4];
    ines.chr_size=(unsigned char)rom[5];
    ines.prg.resize(ines.prg_size*16384);
    ines.chr.resize(ines.chr_size*8192);
    int n=16;
    std::copy(rom.begin()+n,rom.begin()+n+ines.prg_size*16384,ines.prg.begin());
    n+=ines.prg_size*16384;
    std::copy(rom.begin()+n,rom.begin()+n+ines.chr_size*8192,ines.chr.begin());
    return ines;
}
