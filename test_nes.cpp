#include <iostream>
#include <cstdlib>
#include "nes.h"
using namespace std;
int main(){
    INES ines=read_rom("../roms/instr_misc.nes");
    NES nes{};
    nes.ines=ines;
    nes.reset();
    for(int i=0;i<1000;i++){
        try{
        nes.step_cpu();
        }
        catch(const char *e){
            cout<<e<<endl;
            throw;            
        }
    }
}
