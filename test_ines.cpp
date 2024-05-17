#include <iostream>
#include "ines.h"
using namespace std;
int main(){
    INES ines=read_rom("../roms/palette_ram.nes");
    cout<<ines.prg_size<<","<<ines.chr_size<<endl;
}
