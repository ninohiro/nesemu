#include "nes.h"
NES::NES(INES ines,uint32_t *pixels):cpu{},ppu{},ines{ines},pixels{pixels}{
    cpu.S=0xFD;
    unsigned short a1=(unsigned short)load_cpu_mem(0xFFFC)+(unsigned short)load_cpu_mem(0xFFFD)*256;
    cpu.PC=a1;
    cpu.P=32|4;
}

unsigned char NES::load_cpu_mem(unsigned short addr){
    if(addr<=0x1FFF){
        return cpu.ram[addr&0x07FF];
    }
    else if(addr>=0x2000 && addr<=0x3FFF){
        addr=(addr&0x7)|0x2000;
        if(addr==0x2002){
            ppu.write_toggle=false;
        }
        if(addr==0x2007){
            unsigned char v=load_ppu_mem(ppu.internal_addr);
            if(ppu.registers[0]&4){
                ppu.internal_addr+=32;
            }
            else{
                ppu.internal_addr+=1;
            }
            return v;
        }
        else{
            return ppu.registers[addr-0x2000];
        }
    }
    else if(addr>=0x4000 && addr<=0x401F){
        return io[addr-0x4000];
    }
    else if(addr>=0x6000 && addr<=0x7FFF){
        return cpu.prg_ram[addr-0x6000];
    }
    else if(addr>=0x8000){
        return ines.prg[addr-0x8000];
    }
    else{
        throw "load: invalid addr";
    }
}
void NES::store_cpu_mem(unsigned short addr,unsigned char value){
    if(addr<=0x07FF){
        cpu.ram[addr]=value;
    }
    else if(addr>=0x2000 && addr<=0x3FFF){
        addr=(addr&0x7)|0x2000;
        if(addr==0x2005){
            if(!ppu.write_toggle){
                ppu.scroll_x=value;
            }
            else{
                ppu.scroll_y=value;
            }
            ppu.write_toggle=!ppu.write_toggle;
        }
        else if(addr==0x2006){
            if(!ppu.write_toggle){
                ppu.internal_addr=(unsigned short)value*256;
            }
            else{
                ppu.internal_addr+=value;
            }
            ppu.write_toggle=!ppu.write_toggle;
        }
        else if(addr==0x2007){
            store_ppu_mem(ppu.internal_addr,value);
            if(ppu.registers[0]&4){
                ppu.internal_addr+=32;
            }
            else{
                ppu.internal_addr+=1;
            }
        }
        else{
            ppu.registers[addr-0x2000]=value;
        }
    }
    else if(addr>=0x4000 && addr<=0x401F){
        if(addr==0x4014){
            unsigned short start=(unsigned short)value*256;
            for(int i=0;i<256;i++){
                ppu.oam[i]=load_cpu_mem(start+i);
            }
            cpu.dma_wait=513;
        }
        else{
            io[addr-0x4000]=value;
        }
    }
    else if(addr>=0x6000 && addr<=0x7FFF){
        cpu.prg_ram[addr-0x6000]=value;
    }
    else{
        throw "store: invalid addr";
    }
}

void NES::step_cpu(){
    if(cpu.wait>0){
        cpu.wait--;
        return;
    }
    if(cpu.dma_wait>0){
        cpu.dma_wait--;
        return;
    }
    unsigned short a1,a2;
    unsigned char v1,v2;
    unsigned short s1;
    if(pin_nmi){
        a1=cpu.PC;
        cpu.S-=2;
        store_cpu_mem(0x100+((cpu.S+1)&0xff),(unsigned char)a1);
        store_cpu_mem(0x100+((cpu.S+2)&0xff),(unsigned char)(a1>>8));
        cpu.S--;
        store_cpu_mem(0x100+((cpu.S+1)&0xff),~16&cpu.P);
        a2=(unsigned short)load_cpu_mem(0xFFFA)+(unsigned short)load_cpu_mem(0xFFFB)*256;
        cpu.PC=a2;
        cpu.wait=7;
        pin_nmi=0;
        cpu.wait--;
        return;
    }
    if(cpu.PC==0xE2EF){
        cpu.PC=cpu.PC;
    }
    cpu.prev_pc=cpu.PC;
    unsigned char c=load_cpu_mem(cpu.PC);
    switch(c){
    case 0xA9:
        cpu.A=load_cpu_mem(cpu.PC+1);
        cpu.P=(~(128|2)&cpu.P)|(cpu.A&128)|(2*(cpu.A==0));
        cpu.PC+=2;
        cpu.wait=2;
        break;
    case 0xA5:
        a1=load_cpu_mem(cpu.PC+1);
        cpu.A=load_cpu_mem(a1);
        cpu.P=(~(128|2)&cpu.P)|(cpu.A&128)|(2*(cpu.A==0));
        cpu.PC+=2;
        cpu.wait=3;
        break;
    case 0xB5:
        a1=load_cpu_mem(cpu.PC+1)+cpu.X;
        cpu.A=load_cpu_mem(a1);
        cpu.P=(~(128|2)&cpu.P)|(cpu.A&128)|(2*(cpu.A==0));
        cpu.PC+=2;
        cpu.wait=4;
        break;
    case 0xAD:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256;
        cpu.A=load_cpu_mem(a1);
        cpu.P=(~(128|2)&cpu.P)|(cpu.A&128)|(2*(cpu.A==0));
        cpu.PC+=3;
        cpu.wait=4;
        break;
    case 0xBD:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256+cpu.X;
        cpu.A=load_cpu_mem(a1);
        cpu.P=(~(128|2)&cpu.P)|(cpu.A&128)|(2*(cpu.A==0));
        cpu.PC+=3;
        cpu.wait=4;
        break;
    case 0xB9:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256+cpu.Y;
        cpu.A=load_cpu_mem(a1);
        cpu.P=(~(128|2)&cpu.P)|(cpu.A&128)|(2*(cpu.A==0));
        cpu.PC+=3;
        cpu.wait=4;
        break;
    case 0xA1:
        a1=load_cpu_mem(cpu.PC+1)+cpu.X;
        a2=(unsigned short)load_cpu_mem(a1&0xff)+(unsigned short)load_cpu_mem((a1+1)&0xff)*256;
        cpu.A=load_cpu_mem(a2);
        cpu.P=(~(128|2)&cpu.P)|(cpu.A&128)|(2*(cpu.A==0));
        cpu.PC+=2;
        cpu.wait=6;
        break;
    case 0xB1:
        a1=load_cpu_mem(cpu.PC+1);
        a2=(unsigned short)load_cpu_mem(a1&0xff)+(unsigned short)load_cpu_mem((a1+1)&0xff)*256+cpu.Y;
        cpu.A=load_cpu_mem(a2);
        cpu.P=(~(128|2)&cpu.P)|(cpu.A&128)|(2*(cpu.A==0));
        cpu.PC+=2;
        cpu.wait=5;
        break;
    case 0xA2:
        cpu.X=load_cpu_mem(cpu.PC+1);
        cpu.P=(~(128|2)&cpu.P)|(cpu.X&128)|(2*(cpu.X==0));
        cpu.PC+=2;
        cpu.wait=2;
        break;
    case 0xA6:
        a1=load_cpu_mem(cpu.PC+1);
        cpu.X=load_cpu_mem(a1);
        cpu.P=(~(128|2)&cpu.P)|(cpu.X&128)|(2*(cpu.X==0));
        cpu.PC+=2;
        cpu.wait=3;
        break;
    case 0xB6:
        a1=load_cpu_mem(cpu.PC+1)+cpu.Y;
        cpu.X=load_cpu_mem(a1);
        cpu.P=(~(128|2)&cpu.P)|(cpu.X&128)|(2*(cpu.X==0));
        cpu.PC+=2;
        cpu.wait=4;
        break;
    case 0xAE:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256;
        cpu.X=load_cpu_mem(a1);
        cpu.P=(~(128|2)&cpu.P)|(cpu.X&128)|(2*(cpu.X==0));
        cpu.PC+=3;
        cpu.wait=4;
        break;
    case 0xBE:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256+cpu.Y;
        cpu.X=load_cpu_mem(a1);
        cpu.P=(~(128|2)&cpu.P)|(cpu.X&128)|(2*(cpu.X==0));
        cpu.PC+=3;
        cpu.wait=4;
        break;
    case 0xA0:
        cpu.Y=load_cpu_mem(cpu.PC+1);
        cpu.P=(~(128|2)&cpu.P)|(cpu.Y&128)|(2*(cpu.Y==0));
        cpu.PC+=2;
        cpu.wait=2;
        break;
    case 0xA4:
        a1=load_cpu_mem(cpu.PC+1);
        cpu.Y=load_cpu_mem(a1);
        cpu.P=(~(128|2)&cpu.P)|(cpu.Y&128)|(2*(cpu.Y==0));
        cpu.PC+=2;
        cpu.wait=3;
        break;
    case 0xB4:
        a1=load_cpu_mem(cpu.PC+1)+cpu.X;
        cpu.Y=load_cpu_mem(a1);
        cpu.P=(~(128|2)&cpu.P)|(cpu.Y&128)|(2*(cpu.Y==0));
        cpu.PC+=2;
        cpu.wait=4;
        break;
    case 0xAC:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256;
        cpu.Y=load_cpu_mem(a1);
        cpu.P=(~(128|2)&cpu.P)|(cpu.Y&128)|(2*(cpu.Y==0));
        cpu.PC+=3;
        cpu.wait=4;
        break;
    case 0xBC:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256+cpu.X;
        cpu.Y=load_cpu_mem(a1);
        cpu.P=(~(128|2)&cpu.P)|(cpu.Y&128)|(2*(cpu.Y==0));
        cpu.PC+=3;
        cpu.wait=4;
        break;
    case 0x85:
        a1=load_cpu_mem(cpu.PC+1);
        store_cpu_mem(a1,cpu.A);
        cpu.PC+=2;
        cpu.wait=3;
        break;
    case 0x95:
        a1=load_cpu_mem(cpu.PC+1)+cpu.X;
        store_cpu_mem(a1,cpu.A);
        cpu.PC+=2;
        cpu.wait=4;
        break;
    case 0x8D:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256;
        store_cpu_mem(a1,cpu.A);
        cpu.PC+=3;
        cpu.wait=4;
        break;
    case 0x9D:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256+cpu.X;
        store_cpu_mem(a1,cpu.A);
        cpu.PC+=3;
        cpu.wait=5;
        break;
    case 0x99:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256+cpu.Y;
        store_cpu_mem(a1,cpu.A);
        cpu.PC+=3;
        cpu.wait=5;
        break;
    case 0x81:
        a1=load_cpu_mem(cpu.PC+1)+cpu.X;
        a2=(unsigned short)load_cpu_mem(a1&0xff)+(unsigned short)load_cpu_mem((a1+1)&0xff)*256;
        store_cpu_mem(a2,cpu.A);
        cpu.PC+=2;
        cpu.wait=6;
        break;
    case 0x91:
        a1=load_cpu_mem(cpu.PC+1);
        a2=(unsigned short)load_cpu_mem(a1&0xff)+(unsigned short)load_cpu_mem((a1+1)&0xff)*256+cpu.Y;
        store_cpu_mem(a2,cpu.A);
        cpu.PC+=2;
        cpu.wait=6;
        break;
    case 0x86:
        a1=load_cpu_mem(cpu.PC+1);
        store_cpu_mem(a1,cpu.X);
        cpu.PC+=2;
        cpu.wait=3;
        break;
    case 0x96:
        a1=load_cpu_mem(cpu.PC+1)+cpu.Y;
        store_cpu_mem(a1,cpu.X);
        cpu.PC+=2;
        cpu.wait=4;
        break;
    case 0x8E:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256;
        store_cpu_mem(a1,cpu.X);
        cpu.PC+=3;
        cpu.wait=4;
        break;
    case 0x84:
        a1=load_cpu_mem(cpu.PC+1);
        store_cpu_mem(a1,cpu.Y);
        cpu.PC+=2;
        cpu.wait=3;
        break;
    case 0x94:
        a1=load_cpu_mem(cpu.PC+1)+cpu.X;
        store_cpu_mem(a1,cpu.Y);
        cpu.PC+=2;
        cpu.wait=4;
        break;
    case 0x8C:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256;
        store_cpu_mem(a1,cpu.Y);
        cpu.PC+=3;
        cpu.wait=4;
        break;
    case 0xAA:
        cpu.X=cpu.A;
        cpu.P=(~(128|2)&cpu.P)|(cpu.X&128)|(2*(cpu.X==0));
        cpu.PC+=1;
        cpu.wait=2;
        break;
    case 0xA8:
        cpu.Y=cpu.A;
        cpu.P=(~(128|2)&cpu.P)|(cpu.Y&128)|(2*(cpu.Y==0));
        cpu.PC+=1;
        cpu.wait=2;
        break;
    case 0xBA:
        cpu.X=cpu.S;
        cpu.P=(~(128|2)&cpu.P)|(cpu.X&128)|(2*(cpu.X==0));
        cpu.PC+=1;
        cpu.wait=2;
        break;
    case 0x8A:
        cpu.A=cpu.X;
        cpu.P=(~(128|2)&cpu.P)|(cpu.A&128)|(2*(cpu.A==0));
        cpu.PC+=1;
        cpu.wait=2;
        break;
    case 0x9A:
        cpu.S=cpu.X;
        cpu.P=(~(128|2)&cpu.P)|(cpu.S&128)|(2*(cpu.S==0));
        cpu.PC+=1;
        cpu.wait=2;
        break;
    case 0x98:
        cpu.A=cpu.Y;
        cpu.P=(~(128|2)&cpu.P)|(cpu.A&128)|(2*(cpu.A==0));
        cpu.PC+=1;
        cpu.wait=2;
        break;

    case 0x69:
        v1=cpu.A;
        v2=load_cpu_mem(cpu.PC+1);
        cpu.A=v1+v2+(cpu.P&1);
        cpu.P=(~(128|64|2|1)&cpu.P)|(cpu.A&128)|(64*((v1>>7==v2>>7)&&(v1>>7!=cpu.A>>7)))|(2*(cpu.A==0))|(1*((v1|v2)>>7==1&&cpu.A>>7==0));
        cpu.PC+=2;
        cpu.wait=2;
        break;
    case 0x65:
        a1=load_cpu_mem(cpu.PC+1);
        v1=cpu.A;
        v2=load_cpu_mem(a1);
        cpu.A=v1+v2+(cpu.P&1);
        cpu.P=(~(128|64|2|1)&cpu.P)|(cpu.A&128)|(64*((v1>>7==v2>>7)&&(v1>>7!=cpu.A>>7)))|(2*(cpu.A==0))|(1*((v1|v2)>>7==1&&cpu.A>>7==0));
        cpu.PC+=2;
        cpu.wait=3;
        break;
    case 0x75:
        a1=load_cpu_mem(cpu.PC+1)+cpu.X;
        v1=cpu.A;
        v2=load_cpu_mem(a1);
        cpu.A=v1+v2+(cpu.P&1);
        cpu.P=(~(128|64|2|1)&cpu.P)|(cpu.A&128)|(64*((v1>>7==v2>>7)&&(v1>>7!=cpu.A>>7)))|(2*(cpu.A==0))|(1*((v1|v2)>>7==1&&cpu.A>>7==0));
        cpu.PC+=2;
        cpu.wait=4;
        break;
    case 0x6D:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256;
        v1=cpu.A;
        v2=load_cpu_mem(a1);
        cpu.A=v1+v2+(cpu.P&1);
        cpu.P=(~(128|64|2|1)&cpu.P)|(cpu.A&128)|(64*((v1>>7==v2>>7)&&(v1>>7!=cpu.A>>7)))|(2*(cpu.A==0))|(1*((v1|v2)>>7==1&&cpu.A>>7==0));
        cpu.PC+=3;
        cpu.wait=4;
        break;
    case 0x7D:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256+cpu.X;
        v1=cpu.A;
        v2=load_cpu_mem(a1);
        cpu.A=v1+v2+(cpu.P&1);
        cpu.P=(~(128|64|2|1)&cpu.P)|(cpu.A&128)|(64*((v1>>7==v2>>7)&&(v1>>7!=cpu.A>>7)))|(2*(cpu.A==0))|(1*((v1|v2)>>7==1&&cpu.A>>7==0));
        cpu.PC+=3;
        cpu.wait=4;
        break;
    case 0x79:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256+cpu.Y;
        v1=cpu.A;
        v2=load_cpu_mem(a1);
        cpu.A=v1+v2+(cpu.P&1);
        cpu.P=(~(128|64|2|1)&cpu.P)|(cpu.A&128)|(64*((v1>>7==v2>>7)&&(v1>>7!=cpu.A>>7)))|(2*(cpu.A==0))|(1*((v1|v2)>>7==1&&cpu.A>>7==0));
        cpu.PC+=3;
        cpu.wait=4;
        break;
    case 0x61:
        a1=load_cpu_mem(cpu.PC+1)+cpu.X;
        a2=(unsigned short)load_cpu_mem(a1&0xff)+(unsigned short)load_cpu_mem((a1+1)&0xff)*256;
        v1=cpu.A;
        v2=load_cpu_mem(a2);
        cpu.A=v1+v2+(cpu.P&1);
        cpu.P=(~(128|64|2|1)&cpu.P)|(cpu.A&128)|(64*((v1>>7==v2>>7)&&(v1>>7!=cpu.A>>7)))|(2*(cpu.A==0))|(1*((v1|v2)>>7==1&&cpu.A>>7==0));
        cpu.PC+=2;
        cpu.wait=6;
        break;
    case 0x71:
        a1=load_cpu_mem(cpu.PC+1);
        a2=(unsigned short)load_cpu_mem(a1&0xff)+(unsigned short)load_cpu_mem((a1+1)&0xff)*256+cpu.Y;
        v1=cpu.A;
        v2=load_cpu_mem(a2);
        cpu.A=v1+v2+(cpu.P&1);
        cpu.P=(~(128|64|2|1)&cpu.P)|(cpu.A&128)|(64*((v1>>7==v2>>7)&&(v1>>7!=cpu.A>>7)))|(2*(cpu.A==0))|(1*((v1|v2)>>7==1&&cpu.A>>7==0));
        cpu.PC+=2;
        cpu.wait=5;
        break;

    case 0x29:
        cpu.A&=load_cpu_mem(cpu.PC+1);
        cpu.P=(~(128|2)&cpu.P)|(cpu.A&128)|(2*(cpu.A==0));
        cpu.PC+=2;
        cpu.wait=2;
        break;
    case 0x25:
        a1=load_cpu_mem(cpu.PC+1);
        cpu.A&=load_cpu_mem(a1);
        cpu.P=(~(128|2)&cpu.P)|(cpu.A&128)|(2*(cpu.A==0));
        cpu.PC+=2;
        cpu.wait=3;
        break;
    case 0x35:
        a1=load_cpu_mem(cpu.PC+1)+cpu.X;
        cpu.A&=load_cpu_mem(a1);
        cpu.P=(~(128|2)&cpu.P)|(cpu.A&128)|(2*(cpu.A==0));
        cpu.PC+=2;
        cpu.wait=4;
        break;
    case 0x2D:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256;
        cpu.A&=load_cpu_mem(a1);
        cpu.P=(~(128|2)&cpu.P)|(cpu.A&128)|(2*(cpu.A==0));
        cpu.PC+=3;
        cpu.wait=4;
        break;
    case 0x3D:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256+cpu.X;
        cpu.A&=load_cpu_mem(a1);
        cpu.P=(~(128|2)&cpu.P)|(cpu.A&128)|(2*(cpu.A==0));
        cpu.PC+=3;
        cpu.wait=4;
        break;
    case 0x39:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256+cpu.Y;
        cpu.A&=load_cpu_mem(a1);
        cpu.P=(~(128|2)&cpu.P)|(cpu.A&128)|(2*(cpu.A==0));
        cpu.PC+=3;
        cpu.wait=4;
        break;
    case 0x21:
        a1=load_cpu_mem(cpu.PC+1)+cpu.X;
        a2=(unsigned short)load_cpu_mem(a1&0xff)+(unsigned short)load_cpu_mem((a1+1)&0xff)*256;
        cpu.A&=load_cpu_mem(a2);
        cpu.P=(~(128|2)&cpu.P)|(cpu.A&128)|(2*(cpu.A==0));
        cpu.PC+=2;
        cpu.wait=6;
        break;
    case 0x31:
        a1=load_cpu_mem(cpu.PC+1);
        a2=(unsigned short)load_cpu_mem(a1&0xff)+(unsigned short)load_cpu_mem((a1+1)&0xff)*256+cpu.Y;
        cpu.A&=load_cpu_mem(a2);
        cpu.P=(~(128|2)&cpu.P)|(cpu.A&128)|(2*(cpu.A==0));
        cpu.PC+=2;
        cpu.wait=5;
        break;

    case 0x0A:
        cpu.P=(~1&cpu.P)|(1*(cpu.A>>7));
        cpu.A<<=1;
        cpu.P=(~(128|2)&cpu.P)|(cpu.A&128)|(2*(cpu.A==0));
        cpu.PC+=1;
        cpu.wait=2;
        break;
    case 0x06:
        a1=load_cpu_mem(cpu.PC+1);
        v1=load_cpu_mem(a1);
        cpu.P=(~1&cpu.P)|(1*(v1>>7));
        v1<<=1;
        store_cpu_mem(a1,v1);
        cpu.P=(~(128|2)&cpu.P)|(v1&128)|(2*(v1==0));
        cpu.PC+=2;
        cpu.wait=5;
        break;
    case 0x16:
        a1=load_cpu_mem(cpu.PC+1)+cpu.X;
        v1=load_cpu_mem(a1);
        cpu.P=(~1&cpu.P)|(1*(v1>>7));
        v1<<=1;
        store_cpu_mem(a1,v1);
        cpu.P=(~(128|2)&cpu.P)|(v1&128)|(2*(v1==0));
        cpu.PC+=2;
        cpu.wait=6;
        break;
    case 0x0E:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256;
        v1=load_cpu_mem(a1);
        cpu.P=(~1&cpu.P)|(1*(v1>>7));
        v1<<=1;
        store_cpu_mem(a1,v1);
        cpu.P=(~(128|2)&cpu.P)|(v1&128)|(2*(v1==0));
        cpu.PC+=3;
        cpu.wait=6;
        break;
    case 0x1E:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256+cpu.X;
        v1=load_cpu_mem(a1);
        cpu.P=(~1&cpu.P)|(1*(v1>>7));
        v1<<=1;
        store_cpu_mem(a1,v1);
        cpu.P=(~(128|2)&cpu.P)|(v1&128)|(2*(v1==0));
        cpu.PC+=3;
        cpu.wait=7;
        break;

    case 0x24:
        a1=load_cpu_mem(cpu.PC+1);
        v1=load_cpu_mem(a1);
        cpu.P=(~(128|64|2)&cpu.P)|((128|64)&v1)|(2*((v1&cpu.A)==0));
        cpu.PC+=2;
        cpu.wait=3;
        break;
    case 0x2C:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256;
        v1=load_cpu_mem(a1);
        cpu.P=(~(128|64|2)&cpu.P)|((128|64)&v1)|(2*((v1&cpu.A)==0));
        cpu.PC+=3;
        cpu.wait=4;
        break;

    case 0xC9:
        v1=-load_cpu_mem(cpu.PC+1);
        v2=cpu.A+v1;
        cpu.P=(~(128|2|1)&cpu.P)|(128&v2)|(2*(v2==0))|(1*((v1|cpu.A)>>7==1&&v2>>7==0));
        cpu.PC+=2;
        cpu.wait=2;
        break;
    case 0xC5:
        a1=load_cpu_mem(cpu.PC+1);
        v1=-load_cpu_mem(a1);
        v2=cpu.A+v1;
        cpu.P=(~(128|2|1)&cpu.P)|(128&v2)|(2*(v2==0))|(1*((v1|cpu.A)>>7==1&&v2>>7==0));
        cpu.PC+=2;
        cpu.wait=3;
        break;
    case 0xD5:
        a1=load_cpu_mem(cpu.PC+1)+cpu.X;
        v1=-load_cpu_mem(a1);
        v2=cpu.A+v1;
        cpu.P=(~(128|2|1)&cpu.P)|(128&v2)|(2*(v2==0))|(1*((v1|cpu.A)>>7==1&&v2>>7==0));
        cpu.PC+=2;
        cpu.wait=4;
        break;
    case 0xCD:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256;
        v1=-load_cpu_mem(a1);
        v2=cpu.A+v1;
        cpu.P=(~(128|2|1)&cpu.P)|(128&v2)|(2*(v2==0))|(1*((v1|cpu.A)>>7==1&&v2>>7==0));
        cpu.PC+=3;
        cpu.wait=4;
        break;
    case 0xDD:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256+cpu.X;
        v1=-load_cpu_mem(a1);
        v2=cpu.A+v1;
        cpu.P=(~(128|2|1)&cpu.P)|(128&v2)|(2*(v2==0))|(1*((v1|cpu.A)>>7==1&&v2>>7==0));
        cpu.PC+=3;
        cpu.wait=4;
        break;
    case 0xD9:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256+cpu.Y;
        v1=-load_cpu_mem(a1);
        v2=cpu.A+v1;
        cpu.P=(~(128|2|1)&cpu.P)|(128&v2)|(2*(v2==0))|(1*((v1|cpu.A)>>7==1&&v2>>7==0));
        cpu.PC+=3;
        cpu.wait=4;
        break;
    case 0xC1:
        a1=load_cpu_mem(cpu.PC+1)+cpu.X;
        a2=(unsigned short)load_cpu_mem(a1&0xff)+(unsigned short)load_cpu_mem((a1+1)&0xff)*256;
        v1=-load_cpu_mem(a2);
        v2=cpu.A+v1;
        cpu.P=(~(128|2|1)&cpu.P)|(128&v2)|(2*(v2==0))|(1*((v1|cpu.A)>>7==1&&v2>>7==0));
        cpu.PC+=2;
        cpu.wait=6;
        break;
    case 0xD1:
        a1=load_cpu_mem(cpu.PC+1);
        a2=(unsigned short)load_cpu_mem(a1&0xff)+(unsigned short)load_cpu_mem((a1+1)&0xff)*256+cpu.Y;
        v1=-load_cpu_mem(a2);
        v2=cpu.A+v1;
        cpu.P=(~(128|2|1)&cpu.P)|(128&v2)|(2*(v2==0))|(1*((v1|cpu.A)>>7==1&&v2>>7==0));
        cpu.PC+=2;
        cpu.wait=5;
        break;

    case 0xE0:
        v1=-load_cpu_mem(cpu.PC+1);
        v2=cpu.X+v1;
        cpu.P=(~(128|2|1)&cpu.P)|(128&v2)|(2*(v2==0))|(1*((v1|cpu.X)>>7==1&&v2>>7==0));
        cpu.PC+=2;
        cpu.wait=2;
        break;
    case 0xE4:
        a1=load_cpu_mem(cpu.PC+1);
        v1=-load_cpu_mem(a1);
        v2=cpu.X+v1;
        cpu.P=(~(128|2|1)&cpu.P)|(128&v2)|(2*(v2==0))|(1*((v1|cpu.X)>>7==1&&v2>>7==0));
        cpu.PC+=2;
        cpu.wait=3;
        break;
    case 0xEC:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256;
        v1=-load_cpu_mem(a1);
        v2=cpu.X+v1;
        cpu.P=(~(128|2|1)&cpu.P)|(128&v2)|(2*(v2==0))|(1*((v1|cpu.X)>>7==1&&v2>>7==0));
        cpu.PC+=3;
        cpu.wait=4;
        break;

    case 0xC0:
        v1=-load_cpu_mem(cpu.PC+1);
        v2=cpu.Y+v1;
        cpu.P=(~(128|2|1)&cpu.P)|(128&v2)|(2*(v2==0))|(1*((v1|cpu.Y)>>7==1&&v2>>7==0));
        cpu.PC+=2;
        cpu.wait=2;
        break;
    case 0xC4:
        a1=load_cpu_mem(cpu.PC+1);
        v1=-load_cpu_mem(a1);
        v2=cpu.Y+v1;
        cpu.P=(~(128|2|1)&cpu.P)|(128&v2)|(2*(v2==0))|(1*((v1|cpu.Y)>>7==1&&v2>>7==0));
        cpu.PC+=2;
        cpu.wait=3;
        break;
    case 0xCC:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256;
        v1=-load_cpu_mem(a1);
        v2=cpu.Y+v1;
        cpu.P=(~(128|2|1)&cpu.P)|(128&v2)|(2*(v2==0))|(1*((v1|cpu.Y)>>7==1&&v2>>7==0));
        cpu.PC+=3;
        cpu.wait=4;
        break;

    case 0xC6:
        a1=load_cpu_mem(cpu.PC+1);
        v1=load_cpu_mem(a1);
        v1--;
        store_cpu_mem(a1,v1);
        cpu.P=(~(128|2)&cpu.P)|(v1&128)|(2*(v1==0));
        cpu.PC+=2;
        cpu.wait=5;
        break;
    case 0xD6:
        a1=load_cpu_mem(cpu.PC+1)+cpu.X;
        v1=load_cpu_mem(a1);
        v1--;
        store_cpu_mem(a1,v1);
        cpu.P=(~(128|2)&cpu.P)|(v1&128)|(2*(v1==0));
        cpu.PC+=2;
        cpu.wait=6;
        break;
    case 0xCE:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256;
        v1=load_cpu_mem(a1);
        v1--;
        store_cpu_mem(a1,v1);
        cpu.P=(~(128|2)&cpu.P)|(v1&128)|(2*(v1==0));
        cpu.PC+=3;
        cpu.wait=6;
        break;
    case 0xDE:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256+cpu.X;
        v1=load_cpu_mem(a1);
        v1--;
        store_cpu_mem(a1,v1);
        cpu.P=(~(128|2)&cpu.P)|(v1&128)|(2*(v1==0));
        cpu.PC+=3;
        cpu.wait=7;
        break;
    case 0xCA:
        cpu.X--;
        cpu.P=(~(128|2)&cpu.P)|(cpu.X&128)|(2*(cpu.X==0));
        cpu.PC+=1;
        cpu.wait=2;
        break;
    case 0x88:
        cpu.Y--;
        cpu.P=(~(128|2)&cpu.P)|(cpu.Y&128)|(2*(cpu.Y==0));
        cpu.PC+=1;
        cpu.wait=2;
        break;

    case 0x49:
        cpu.A^=load_cpu_mem(cpu.PC+1);
        cpu.P=(~(128|2)&cpu.P)|(cpu.A&128)|(2*(cpu.A==0));
        cpu.PC+=2;
        cpu.wait=2;
        break;
    case 0x45:
        a1=load_cpu_mem(cpu.PC+1);
        cpu.A^=load_cpu_mem(a1);
        cpu.P=(~(128|2)&cpu.P)|(cpu.A&128)|(2*(cpu.A==0));
        cpu.PC+=2;
        cpu.wait=3;
        break;
    case 0x55:
        a1=load_cpu_mem(cpu.PC+1)+cpu.X;
        cpu.A^=load_cpu_mem(a1);
        cpu.P=(~(128|2)&cpu.P)|(cpu.A&128)|(2*(cpu.A==0));
        cpu.PC+=2;
        cpu.wait=4;
        break;
    case 0x4D:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256;
        cpu.A^=load_cpu_mem(a1);
        cpu.P=(~(128|2)&cpu.P)|(cpu.A&128)|(2*(cpu.A==0));
        cpu.PC+=3;
        cpu.wait=4;
        break;
    case 0x5D:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256+cpu.X;
        cpu.A^=load_cpu_mem(a1);
        cpu.P=(~(128|2)&cpu.P)|(cpu.A&128)|(2*(cpu.A==0));
        cpu.PC+=3;
        cpu.wait=4;
        break;
    case 0x59:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256+cpu.Y;
        cpu.A^=load_cpu_mem(a1);
        cpu.P=(~(128|2)&cpu.P)|(cpu.A&128)|(2*(cpu.A==0));
        cpu.PC+=3;
        cpu.wait=4;
        break;
    case 0x41:
        a1=load_cpu_mem(cpu.PC+1)+cpu.X;
        a2=(unsigned short)load_cpu_mem(a1&0xff)+(unsigned short)load_cpu_mem((a1+1)&0xff)*256;
        cpu.A^=load_cpu_mem(a2);
        cpu.P=(~(128|2)&cpu.P)|(cpu.A&128)|(2*(cpu.A==0));
        cpu.PC+=2;
        cpu.wait=6;
        break;
    case 0x51:
        a1=load_cpu_mem(cpu.PC+1);
        a2=(unsigned short)load_cpu_mem(a1&0xff)+(unsigned short)load_cpu_mem((a1+1)&0xff)*256+cpu.Y;
        cpu.A^=load_cpu_mem(a2);
        cpu.P=(~(128|2)&cpu.P)|(cpu.A&128)|(2*(cpu.A==0));
        cpu.PC+=2;
        cpu.wait=5;
        break;

    case 0xE6:
        a1=load_cpu_mem(cpu.PC+1);
        v1=load_cpu_mem(a1);
        v1++;
        store_cpu_mem(a1,v1);
        cpu.P=(~(128|2)&cpu.P)|(v1&128)|(2*(v1==0));
        cpu.PC+=2;
        cpu.wait=5;
        break;
    case 0xF6:
        a1=load_cpu_mem(cpu.PC+1)+cpu.X;
        v1=load_cpu_mem(a1);
        v1++;
        store_cpu_mem(a1,v1);
        cpu.P=(~(128|2)&cpu.P)|(v1&128)|(2*(v1==0));
        cpu.PC+=2;
        cpu.wait=6;
        break;
    case 0xEE:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256;
        v1=load_cpu_mem(a1);
        v1++;
        store_cpu_mem(a1,v1);
        cpu.P=(~(128|2)&cpu.P)|(v1&128)|(2*(v1==0));
        cpu.PC+=3;
        cpu.wait=6;
        break;
    case 0xFE:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256+cpu.X;
        v1=load_cpu_mem(a1);
        v1++;
        store_cpu_mem(a1,v1);
        cpu.P=(~(128|2)&cpu.P)|(v1&128)|(2*(v1==0));
        cpu.PC+=3;
        cpu.wait=7;
        break;

    case 0xE8:
        cpu.X++;
        cpu.P=(~(128|2)&cpu.P)|(cpu.X&128)|(2*(cpu.X==0));
        cpu.PC+=1;
        cpu.wait=2;
        break;
    case 0xC8:
        cpu.Y++;
        cpu.P=(~(128|2)&cpu.P)|(cpu.Y&128)|(2*(cpu.Y==0));
        cpu.PC+=1;
        cpu.wait=2;
        break;

    case 0x4A:
        cpu.P=(~1&cpu.P)|(1*(cpu.A&1));
        cpu.A>>=1;
        cpu.P=(~(128|2)&cpu.P)|(cpu.A&128)|(2*(cpu.A==0));
        cpu.PC+=1;
        cpu.wait=2;
        break;
    case 0x46:
        a1=load_cpu_mem(cpu.PC+1);
        v1=load_cpu_mem(a1);
        cpu.P=(~1&cpu.P)|(1*(v1&1));
        v1>>=1;
        store_cpu_mem(a1,v1);
        cpu.P=(~(128|2)&cpu.P)|(v1&128)|(2*(v1==0));
        cpu.PC+=2;
        cpu.wait=5;
        break;
    case 0x56:
        a1=load_cpu_mem(cpu.PC+1)+cpu.X;
        v1=load_cpu_mem(a1);
        cpu.P=(~1&cpu.P)|(1*(v1&1));
        v1>>=1;
        store_cpu_mem(a1,v1);
        cpu.P=(~(128|2)&cpu.P)|(v1&128)|(2*(v1==0));
        cpu.PC+=2;
        cpu.wait=6;
        break;
    case 0x4E:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256;
        v1=load_cpu_mem(a1);
        cpu.P=(~1&cpu.P)|(1*(v1&1));
        v1>>=1;
        store_cpu_mem(a1,v1);
        cpu.P=(~(128|2)&cpu.P)|(v1&128)|(2*(v1==0));
        cpu.PC+=3;
        cpu.wait=6;
        break;
    case 0x5E:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256+cpu.X;
        v1=load_cpu_mem(a1);
        cpu.P=(~1&cpu.P)|(1*(v1&1));
        v1>>=1;
        store_cpu_mem(a1,v1);
        cpu.P=(~(128|2)&cpu.P)|(v1&128)|(2*(v1==0));
        cpu.PC+=3;
        cpu.wait=7;
        break;

    case 0x09:
        cpu.A|=load_cpu_mem(cpu.PC+1);
        cpu.P=(~(128|2)&cpu.P)|(cpu.A&128)|(2*(cpu.A==0));
        cpu.PC+=2;
        cpu.wait=2;
        break;
    case 0x05:
        a1=load_cpu_mem(cpu.PC+1);
        cpu.A|=load_cpu_mem(a1);
        cpu.P=(~(128|2)&cpu.P)|(cpu.A&128)|(2*(cpu.A==0));
        cpu.PC+=2;
        cpu.wait=3;
        break;
    case 0x15:
        a1=load_cpu_mem(cpu.PC+1)+cpu.X;
        cpu.A|=load_cpu_mem(a1);
        cpu.P=(~(128|2)&cpu.P)|(cpu.A&128)|(2*(cpu.A==0));
        cpu.PC+=2;
        cpu.wait=4;
        break;
    case 0x0D:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256;
        cpu.A|=load_cpu_mem(a1);
        cpu.P=(~(128|2)&cpu.P)|(cpu.A&128)|(2*(cpu.A==0));
        cpu.PC+=3;
        cpu.wait=4;
        break;
    case 0x1D:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256+cpu.X;
        cpu.A|=load_cpu_mem(a1);
        cpu.P=(~(128|2)&cpu.P)|(cpu.A&128)|(2*(cpu.A==0));
        cpu.PC+=3;
        cpu.wait=4;
        break;
    case 0x19:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256+cpu.Y;
        cpu.A|=load_cpu_mem(a1);
        cpu.P=(~(128|2)&cpu.P)|(cpu.A&128)|(2*(cpu.A==0));
        cpu.PC+=3;
        cpu.wait=4;
        break;
    case 0x01:
        a1=load_cpu_mem(cpu.PC+1)+cpu.X;
        a2=(unsigned short)load_cpu_mem(a1&0xff)+(unsigned short)load_cpu_mem((a1+1)&0xff)*256;
        cpu.A|=load_cpu_mem(a2);
        cpu.P=(~(128|2)&cpu.P)|(cpu.A&128)|(2*(cpu.A==0));
        cpu.PC+=2;
        cpu.wait=6;
        break;
    case 0x11:
        a1=load_cpu_mem(cpu.PC+1);
        a2=(unsigned short)load_cpu_mem(a1&0xff)+(unsigned short)load_cpu_mem((a1+1)&0xff)*256+cpu.Y;
        cpu.A|=load_cpu_mem(a2);
        cpu.P=(~(128|2)&cpu.P)|(cpu.A&128)|(2*(cpu.A==0));
        cpu.PC+=2;
        cpu.wait=5;
        break;

    case 0x2A:
        v2=cpu.P&1;
        cpu.P=(~1&cpu.P)|(1*(cpu.A>>7));
        cpu.A<<=1;
        cpu.A|=v2;
        cpu.P=(~(128|2)&cpu.P)|(cpu.A&128)|(2*(cpu.A==0));
        cpu.PC+=1;
        cpu.wait=2;
        break;
    case 0x26:
        a1=load_cpu_mem(cpu.PC+1);
        v1=load_cpu_mem(a1);
        v2=cpu.P&1;
        cpu.P=(~1&cpu.P)|(1*(v1>>7));
        v1<<=1;
        v1|=v2;
        store_cpu_mem(a1,v1);
        cpu.P=(~(128|2)&cpu.P)|(v1&128)|(2*(v1==0));
        cpu.PC+=2;
        cpu.wait=5;
        break;
    case 0x36:
        a1=load_cpu_mem(cpu.PC+1)+cpu.X;
        v1=load_cpu_mem(a1);
        v2=cpu.P&1;
        cpu.P=(~1&cpu.P)|(1*(v1>>7));
        v1<<=1;
        v1|=v2;
        store_cpu_mem(a1,v1);
        cpu.P=(~(128|2)&cpu.P)|(v1&128)|(2*(v1==0));
        cpu.PC+=2;
        cpu.wait=6;
        break;
    case 0x2E:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256;
        v1=load_cpu_mem(a1);
        v2=cpu.P&1;
        cpu.P=(~1&cpu.P)|(1*(v1>>7));
        v1<<=1;
        v1|=v2;
        store_cpu_mem(a1,v1);
        cpu.P=(~(128|2)&cpu.P)|(v1&128)|(2*(v1==0));
        cpu.PC+=3;
        cpu.wait=6;
        break;
    case 0x3E:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256+cpu.X;
        v1=load_cpu_mem(a1);
        v2=cpu.P&1;
        cpu.P=(~1&cpu.P)|(1*(v1>>7));
        v1<<=1;
        v1|=v2;
        store_cpu_mem(a1,v1);
        cpu.P=(~(128|2)&cpu.P)|(v1&128)|(2*(v1==0));
        cpu.PC+=3;
        cpu.wait=7;
        break;

    case 0x6A:
        v2=cpu.P&1;
        cpu.P=(~1&cpu.P)|(1*(cpu.A&1));
        cpu.A>>=1;
        cpu.A|=v2<<7;
        cpu.P=(~(128|2)&cpu.P)|(cpu.A&128)|(2*(cpu.A==0));
        cpu.PC+=1;
        cpu.wait=2;
        break;
    case 0x66:
        a1=load_cpu_mem(cpu.PC+1);
        v1=load_cpu_mem(a1);
        v2=cpu.P&1;
        cpu.P=(~1&cpu.P)|(1*(v1&1));
        v1>>=1;
        v1|=v2<<7;
        store_cpu_mem(a1,v1);
        cpu.P=(~(128|2)&cpu.P)|(v1&128)|(2*(v1==0));
        cpu.PC+=2;
        cpu.wait=5;
        break;
    case 0x76:
        a1=load_cpu_mem(cpu.PC+1)+cpu.X;
        v1=load_cpu_mem(a1);
        v2=cpu.P&1;
        cpu.P=(~1&cpu.P)|(1*(v1&1));
        v1>>=1;
        v1|=v2<<7;
        store_cpu_mem(a1,v1);
        cpu.P=(~(128|2)&cpu.P)|(v1&128)|(2*(v1==0));
        cpu.PC+=2;
        cpu.wait=6;
        break;
    case 0x6E:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256;
        v1=load_cpu_mem(a1);
        v2=cpu.P&1;
        cpu.P=(~1&cpu.P)|(1*(v1&1));
        v1>>=1;
        v1|=v2<<7;
        store_cpu_mem(a1,v1);
        cpu.P=(~(128|2)&cpu.P)|(v1&128)|(2*(v1==0));
        cpu.PC+=3;
        cpu.wait=6;
        break;
    case 0x7E:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256+cpu.X;
        v1=load_cpu_mem(a1);
        v2=cpu.P&1;
        cpu.P=(~1&cpu.P)|(1*(v1&1));
        v1>>=1;
        v1|=v2<<7;
        store_cpu_mem(a1,v1);
        cpu.P=(~(128|2)&cpu.P)|(v1&128)|(2*(v1==0));
        cpu.PC+=3;
        cpu.wait=7;
        break;

    case 0xE9:
        v1=cpu.A;
        v2=~load_cpu_mem(cpu.PC+1);
        cpu.A=v1+v2+(cpu.P&1);
        cpu.P=(~(128|64|2|1)&cpu.P)|(cpu.A&128)|(64*((v1>>7==v2>>7)&&(v1>>7!=cpu.A>>7)))|(2*(cpu.A==0))|(1*((v1|v2)>>7==1&&cpu.A>>7==0));
        cpu.PC+=2;
        cpu.wait=2;
        break;
    case 0xE5:
        a1=load_cpu_mem(cpu.PC+1);
        v1=cpu.A;
        v2=~load_cpu_mem(a1);
        cpu.A=v1+v2+(cpu.P&1);
        cpu.P=(~(128|64|2|1)&cpu.P)|(cpu.A&128)|(64*((v1>>7==v2>>7)&&(v1>>7!=cpu.A>>7)))|(2*(cpu.A==0))|(1*((v1|v2)>>7==1&&cpu.A>>7==0));
        cpu.PC+=2;
        cpu.wait=3;
        break;
    case 0xF5:
        a1=load_cpu_mem(cpu.PC+1)+cpu.X;
        v1=cpu.A;
        v2=~load_cpu_mem(a1);
        cpu.A=v1+v2+(cpu.P&1);
        cpu.P=(~(128|64|2|1)&cpu.P)|(cpu.A&128)|(64*((v1>>7==v2>>7)&&(v1>>7!=cpu.A>>7)))|(2*(cpu.A==0))|(1*((v1|v2)>>7==1&&cpu.A>>7==0));
        cpu.PC+=2;
        cpu.wait=4;
        break;
    case 0xED:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256;
        v1=cpu.A;
        v2=~load_cpu_mem(a1);
        cpu.A=v1+v2+(cpu.P&1);
        cpu.P=(~(128|64|2|1)&cpu.P)|(cpu.A&128)|(64*((v1>>7==v2>>7)&&(v1>>7!=cpu.A>>7)))|(2*(cpu.A==0))|(1*((v1|v2)>>7==1&&cpu.A>>7==0));
        cpu.PC+=3;
        cpu.wait=4;
        break;
    case 0xFD:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256+cpu.X;
        v1=cpu.A;
        v2=~load_cpu_mem(a1);
        cpu.A=v1+v2+(cpu.P&1);
        cpu.P=(~(128|64|2|1)&cpu.P)|(cpu.A&128)|(64*((v1>>7==v2>>7)&&(v1>>7!=cpu.A>>7)))|(2*(cpu.A==0))|(1*((v1|v2)>>7==1&&cpu.A>>7==0));
        cpu.PC+=3;
        cpu.wait=4;
        break;
    case 0xF9:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256+cpu.Y;
        v1=cpu.A;
        v2=~load_cpu_mem(a1);
        cpu.A=v1+v2+(cpu.P&1);
        cpu.P=(~(128|64|2|1)&cpu.P)|(cpu.A&128)|(64*((v1>>7==v2>>7)&&(v1>>7!=cpu.A>>7)))|(2*(cpu.A==0))|(1*((v1|v2)>>7==1&&cpu.A>>7==0));
        cpu.PC+=3;
        cpu.wait=4;
        break;
    case 0xE1:
        a1=load_cpu_mem(cpu.PC+1)+cpu.X;
        a2=(unsigned short)load_cpu_mem(a1&0xff)+(unsigned short)load_cpu_mem((a1+1)&0xff)*256;
        v1=cpu.A;
        v2=~load_cpu_mem(a2);
        cpu.A=v1+v2+(cpu.P&1);
        cpu.P=(~(128|64|2|1)&cpu.P)|(cpu.A&128)|(64*((v1>>7==v2>>7)&&(v1>>7!=cpu.A>>7)))|(2*(cpu.A==0))|(1*((v1|v2)>>7==1&&cpu.A>>7==0));
        cpu.PC+=2;
        cpu.wait=6;
        break;
    case 0xF1:
        a1=load_cpu_mem(cpu.PC+1);
        a2=(unsigned short)load_cpu_mem(a1&0xff)+(unsigned short)load_cpu_mem((a1+1)&0xff)*256+cpu.Y;
        v1=cpu.A;
        v2=~load_cpu_mem(a2);
        cpu.A=v1+v2+(cpu.P&1);
        cpu.P=(~(128|64|2|1)&cpu.P)|(cpu.A&128)|(64*((v1>>7==v2>>7)&&(v1>>7!=cpu.A>>7)))|(2*(cpu.A==0))|(1*((v1|v2)>>7==1&&cpu.A>>7==0));
        cpu.PC+=2;
        cpu.wait=5;
        break;

    case 0x48:
        cpu.S--;
        store_cpu_mem(0x100+((cpu.S+1)&0xff),cpu.A);
        cpu.PC+=1;
        cpu.wait=3;
        break;
    case 0x08:
        cpu.S--;
        store_cpu_mem(0x100+((cpu.S+1)&0xff),cpu.P|16);
        cpu.PC+=1;
        cpu.wait=3;
        break;

    case 0x68:
        cpu.A=load_cpu_mem(0x100+((cpu.S+1)&0xff));
        cpu.S++;
        cpu.P=(~(128|2)&cpu.P)|(cpu.A&128)|(2*(cpu.A==0));
        cpu.PC+=1;
        cpu.wait=4;
        break;
    case 0x28:
        cpu.P=32|((~16)&load_cpu_mem(0x100+((cpu.S+1)&0xff)));
        cpu.S++;
        cpu.PC+=1;
        cpu.wait=4;
        break;

    case 0x4C:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256;
        cpu.PC=a1;
        cpu.wait=3;
        break;
    case 0x6C:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256;
        a2=(unsigned short)load_cpu_mem(a1)+(unsigned short)load_cpu_mem(a1+1)*256;
        cpu.PC=a2;
        cpu.wait=5;
        break;

    case 0x20:
        a1=(unsigned short)load_cpu_mem(cpu.PC+1)+(unsigned short)load_cpu_mem(cpu.PC+2)*256;
        a2=cpu.PC+2;
        cpu.S-=2;
        store_cpu_mem(0x100+((cpu.S+1)&0xff),(unsigned char)a2);
        store_cpu_mem(0x100+((cpu.S+2)&0xff),(unsigned char)(a2>>8));
        cpu.PC=a1;
        cpu.wait=6;
        break;
    case 0x60:
        a1=load_cpu_mem(0x100+((cpu.S+1)&0xff))+load_cpu_mem(0x100+((cpu.S+2)&0xff))*256+1;
        cpu.S+=2;
        cpu.PC=a1;
        cpu.wait=6;
        break;
    case 0x40:
        cpu.P=32|((~16)&load_cpu_mem(0x100+((cpu.S+1)&0xff)));
        cpu.S++;
        a1=load_cpu_mem(0x100+((cpu.S+1)&0xff))+load_cpu_mem(0x100+((cpu.S+2)&0xff))*256;
        cpu.S+=2;
        cpu.PC=a1;
        cpu.wait=6;
        break;

    case 0x90:
        a1=cpu.PC+2+(char)load_cpu_mem(cpu.PC+1);
        if((cpu.P&1)==0){
            cpu.PC=a1;
        }
        else{
            cpu.PC+=2;
        }
        cpu.wait=2;
        break;
    case 0xB0:
        a1=cpu.PC+2+(char)load_cpu_mem(cpu.PC+1);
        if((cpu.P&1)==1){
            cpu.PC=a1;
        }
        else{
            cpu.PC+=2;
        }
        cpu.wait=2;
        break;
    case 0xF0:
        a1=cpu.PC+2+(char)load_cpu_mem(cpu.PC+1);
        if((cpu.P&2)>>1==1){
            cpu.PC=a1;
        }
        else{
            cpu.PC+=2;
        }
        cpu.wait=2;
        break;
    case 0x30:
        a1=cpu.PC+2+(char)load_cpu_mem(cpu.PC+1);
        if((cpu.P&128)>>7==1){
            cpu.PC=a1;
        }
        else{
            cpu.PC+=2;
        }
        cpu.wait=2;
        break;
    case 0xD0:
        a1=cpu.PC+2+(char)load_cpu_mem(cpu.PC+1);
        if((cpu.P&2)>>1==0){
            cpu.PC=a1;
        }
        else{
            cpu.PC+=2;
        }
        cpu.wait=2;
        break;
    case 0x10:
        a1=cpu.PC+2+(char)load_cpu_mem(cpu.PC+1);
        if((cpu.P&128)>>7==0){
            cpu.PC=a1;
        }
        else{
            cpu.PC+=2;
        }
        cpu.wait=2;
        break;
    case 0x50:
        a1=cpu.PC+2+(char)load_cpu_mem(cpu.PC+1);
        if((cpu.P&64)>>6==0){
            cpu.PC=a1;
        }
        else{
            cpu.PC+=2;
        }
        cpu.wait=2;
        break;
    case 0x70:
        a1=cpu.PC+2+(char)load_cpu_mem(cpu.PC+1);
        if((cpu.P&64)>>6==1){
            cpu.PC=a1;
        }
        else{
            cpu.PC+=2;
        }
        cpu.wait=2;
        break;

    case 0x18:
        cpu.P=(~(1)&cpu.P);
        cpu.PC+=1;
        cpu.wait=2;
        break;
    case 0x58:
        cpu.P=(~(4)&cpu.P);
        cpu.PC+=1;
        cpu.wait=2;
        break;
    case 0xB8:
        cpu.P=(~(64)&cpu.P);
        cpu.PC+=1;
        cpu.wait=2;
        break;
    case 0x38:
        cpu.P=(~(1)&cpu.P)|1;
        cpu.PC+=1;
        cpu.wait=2;
        break;
    case 0x78:
        cpu.P=(~(4)&cpu.P)|4;
        cpu.PC+=1;
        cpu.wait=2;
        break;

    case 0x00:
        a1=cpu.PC+2;
        cpu.S-=2;
        store_cpu_mem(0x100+((cpu.S+1)&0xff),(unsigned char)a1);
        store_cpu_mem(0x100+((cpu.S+2)&0xff),(unsigned char)(a1>>8));
        cpu.S--;
        store_cpu_mem(0x100+((cpu.S+1)&0xff),cpu.P|16);
        a2=(unsigned short)load_cpu_mem(0xFFFE)+(unsigned short)load_cpu_mem(0xFFFF)*256;
        cpu.PC=a2;
        cpu.wait=7;
        break;

    case 0xF8:
        cpu.P|=8;
        cpu.PC+=1;
        cpu.wait=2;
        break;
    case 0xD8:
        cpu.P=~8&cpu.P;
        cpu.PC+=1;
        cpu.wait=2;
        break;
    case 0xEA:
        cpu.PC+=1;
        cpu.wait=2;
        break;
    default:
        throw "Unknown instruction";
    }
    cpu.wait--;
    cpu.prev_instr=c;
}
