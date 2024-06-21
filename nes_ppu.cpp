#include "nes.h"
unsigned char NES::load_ppu_mem(unsigned short addr){
    if(addr<=0x1FFF){
        if(ines.chr_size>0){
        return ines.chr[addr];
        }
        else{
            return ppu.chr_ram[addr];
        }
    }
    else if(addr>=0x2000 && addr<=0x2FFF){
        //return addr&0xFF;
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
    if(ines.chr_size==0&&addr<=0x1FFF){
        ppu.chr_ram[addr]=value;
    }
    else if(addr>=0x2000 && addr<=0x2FFF){
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
void NES::render_fb(){
    for(int k=0;k<4;k++){
        for(int i=0;i<30;i++){
            for(int j=0;j<32;j++){
                unsigned short a=0x2000+k*0x400+i*0x20+j;
                unsigned char c=load_ppu_mem(a);
                for(int y=0;y<8;y++){
                    for(int x=0;x<8;x++){
                        unsigned char p1=load_ppu_mem(((ppu.registers[0]>>4)&1)*0x1000+c*16+y),p2=load_ppu_mem(((ppu.registers[0]>>4)&1)*0x1000+c*16+y+8);
                        unsigned char b=(p1>>(7-x)&1)|(((p2>>(7-x)&1))<<1);
                        unsigned int table[4]={0xFF000000,0xFF0000FF,0xFF00FF00,0xFFFFFFFF};
                        fb[(k>>1)*240+i*8+y][(k&1)*256+j*8+x]=table[b];
                    }
                }
            }
        }
    }
}
void NES::copy_fb(){
    int x=ppu.scroll_x|((ppu.registers[0]&1)<<8);
    int y=ppu.scroll_y|(((ppu.registers[0]>>1)&1)<<8);
    for(int i=0;i<240;i++){
        for(int j=0;j<256;j++){
            pixels[i*256+j]=fb[i+y][j+x];
        }
    }
}
