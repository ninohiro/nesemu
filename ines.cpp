#include <cstdlib>
#include <vector>
#include <string>
#include <fstream>
#include <iterator>
#include "ines.h"
INES read_rom(const std::string &s){
    INES ines;
    std::basic_ifstream<unsigned char> ifs(s,std::ios_base::in|std::ios_base::binary);
    std::istreambuf_iterator<unsigned char> it(ifs);
    std::istreambuf_iterator<unsigned char> last;
    std::vector<unsigned char> rom(it, last);
    ines.prg_size=rom[4];
    ines.chr_size=rom[5];
    ines.prg.resize(ines.prg_size*16384);
    ines.chr.resize(ines.chr_size*8192);
    int n=16;
    std::copy(rom.begin()+n,rom.begin()+n+ines.prg_size*16384,ines.prg.begin());
    n+=ines.prg_size*16384;
    std::copy(rom.begin()+n,rom.begin()+n+ines.chr_size*8192,ines.chr.begin());
    return ines;
}
