#include "nes.h"
uint32_t palette[0x40]={
    0x626262,0x001FB2,0x2404C8,0x5200B2,0x730076,0x800024,0x730B00,0x522800,
    0x244400,0x005700,0x005C00,0x005324,0x003C76,0x000000,0x000000,0x000000,
    0xABABAB,0x0D57FF,0x4B30FF,0x8A13FF,0xBC08D6,0xD21269,0xC72E00,0x9D5400,
    0x607B00,0x209800,0x00A300,0x009942,0x007DB4,0x000000,0x000000,0x000000,
    0xFFFFFF,0x53AEFF,0x9085FF,0xD365FF,0xFF57FF,0xFF5DCF,0xFF7757,0xFA9E00,
    0xBDC700,0x7AE700,0x43F611,0x26EF7E,0x2CD5F6,0x4E4E4E,0x000000,0x000000,
    0xFFFFFF,0xB6E1FF,0xCED1FF,0xE9C3FF,0xFFBCFF,0xFFBDF4,0xFFC6C3,0xFFD59A,
    0xE9E681,0xCEF481,0xB6FB9A,0xA9FAC3,0xA9F0F4,0xB8B8B8,0x000000,0x000000
};
unsigned char NES::load_ppu_mem(unsigned short addr){
    addr=addr&0x3FFF;
    if(addr<=0x1FFF){
        return (ines.chr_size>0)?ines.chr[addr]:ines.chr_ram[addr];
    }
    else if(addr>=0x2000 && addr<=0x2FFF){
        addr=0x3FF&addr|((ines.flag&1)?(addr&(1<<10)):(addr&(1<<11))>>1);
        return ppu.vram[addr];
    }
    else if(addr>=0x3F00){
        addr&=0x1F;
        if(addr==0x10||addr==0x14||addr==0x18||addr==0x1C){
            addr-=0x10;
        }
        return ppu.palette_ram[addr];
    }
    else{
        throw "load: invalid addr";
    }
}
void NES::store_ppu_mem(unsigned short addr,unsigned char value){
    addr=addr&0x3FFF;
    if(ines.chr_size==0&&addr<=0x1FFF){
        ines.chr_ram[addr]=value;
    }
    else if(addr>=0x2000 && addr<=0x2FFF){
        addr=0x3FF&addr|((ines.flag&1)?addr&(1<<10):(addr&(1<<11))>>1);
        ppu.vram[addr]=value;;
    }
    else if(addr>=0x3F00){
        addr&=0x1F;
        if(addr==0x10||addr==0x14||addr==0x18||addr==0x1C){
            addr-=0x10;
        }
        ppu.palette_ram[addr]=value;
    }
    else{
        //throw "store: invalid addr";
    }
}
void NES::step_ppu(){
    if(ppu.counter==341*262){
        ppu.odd_even=!ppu.odd_even;
        ppu.counter=ppu.odd_even;
    }
    if(ppu.counter==341*241+1){
        ppu.registers[2]|=1<<7;
        if(ppu.registers[0]>>7){
            pin_nmi=1;
        }
    }
    if(ppu.counter==341*261+1){
        ppu.registers[2]=~(3<<6)&ppu.registers[2];
    }
    if(pixels!=nullptr){
        render();
    }
    ppu.counter++;
}
int NES::load_chr(int bank,int n,int c_x,int c_y){
    unsigned char p1=load_ppu_mem(bank*0x1000+n*16+c_y),p2=load_ppu_mem(bank*0x1000+n*16+c_y+8);
    int b=(p1>>(7-c_x)&1)|(((p2>>(7-c_x)&1))<<1);
    return b;
}
void NES::render(){
    uint32_t color=0xFF000000|palette[ppu.palette_ram[0]&0x3F];
    int d_x=ppu.counter%341,d_y=ppu.counter/341;
    if(d_x>=256||d_y>=240){
        return;
    }
    int x=((ppu.scroll_x|((ppu.registers[0]&1)<<8))+d_x)%512;
    int y=((ppu.scroll_y|(((ppu.registers[0]>>1)&1)<<8))+d_y)%480;
    int k=(x/256)|((y/240)<<1);
    int j=(x-x/256*256)/8,i=(y-y/240*240)/8;
    int c_x=x%8,c_y=y%8;

    int bg_b=0;
    if(ppu.registers[0x1]&8){
        bg_b=load_chr((ppu.registers[0]>>4)&1,load_ppu_mem(0x2000+k*0x400+i*0x20+j),x%8,y%8);
        int pal=(load_ppu_mem(0x23C0+k*0x400+(i/4)*8+j/4)>>(j/2%2*2+i/2%2*4))&3;
        if(bg_b!=0){
            color=0xFF000000|palette[ppu.palette_ram[pal*4+bg_b]&0x3F];
        }
    }
    if((ppu.registers[0x1]&16)&&d_y>0&&ppu.oam_list_size[(d_y-1)/8][d_x/8]>0){
        for(j=0;j<ppu.oam_list_size[(d_y-1)/8][d_x/8];j++){
            i=ppu.oam_list[(d_y-1)/8][d_x/8][j]*4;
            if(ppu.oam[i+3]<=d_x&&d_x<ppu.oam[i+3]+8&&ppu.oam[i]+1<=d_y&&d_y<ppu.oam[i]+1+8){
                unsigned char c_x=d_x-ppu.oam[i+3],c_y=d_y-ppu.oam[i]-1;
                if(ppu.oam[i+2]&(1<<6)){
                    c_x=7-c_x;
                }
                if(ppu.oam[i+2]&(1<<7)){
                    c_y=7-c_y;
                }
                int b=load_chr((ppu.registers[0]>>3)&1,ppu.oam[i+1],c_x,c_y);
                if(b!=0){
                    if(bg_b==0||!(ppu.oam[i+2]&(1<<5))){
                        color=0xFF000000|palette[ppu.palette_ram[((ppu.oam[i+2]&3)+4)*4+b]&0x3F];
                    }
                    if(i==0&&b!=0&&bg_b!=0){
                        ppu.registers[2]|=1<<6;
                    }
                    break;
                }
            }
        }
    }
    pixels[d_y*256+d_x]=color;

}
