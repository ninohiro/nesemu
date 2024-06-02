#include "nes.h"
unsigned char NES::load_ppu_mem(unsigned short addr){
    if(addr<=0x1FFF){
        return ines.chr[addr];
    }
    else if(addr>=0x2000 && addr<=0x2FFF){
        unsigned short a=addr&0x3FF;
        if(ines.flag&1){
            a|=addr&(1<<10);
        }
        else{
            a|=(addr&(1<<11))>>1;
        }
        return ppu.vram[a];
    }
    else if(addr>=0x3F00){
        return ppu.palette_ram[addr&0x1F];
    }
    else{
        throw "load: invalid addr";
    }
}
void NES::store_ppu_mem(unsigned short addr,unsigned char value){
    if(addr>=0x2000 && addr<=0x2FFF){
        unsigned short a=addr&0x3FF;
        if(ines.flag&1){
            a|=addr&(1<<10);
        }
        else{
            a|=(addr&(1<<11))>>1;
        }
        ppu.vram[a]=value;;
    }
    else if(addr>=0x3F00){
        ppu.palette_ram[addr&0x1F]=value;
    }
    else{
        throw "load: invalid addr";
    }
}
void NES::step_ppu(){
    if(ppu.counter==341*262-(((ppu.registers[1]>>3)|(ppu.registers[1]>>4))&1)*ppu.odd_even){
        ppu.counter=0;
        ppu.odd_even=!ppu.odd_even;
    }
    if(ppu.counter==341*241+1){
        ppu.registers[2]|=1<<7;
        if(ppu.registers[0]>>7){
            pin_nmi=1;
        }
    }
    if(ppu.counter==341*261+1){
        ppu.registers[2]=~(1<<7)&ppu.registers[2];
    }
    ppu.counter++;
}
