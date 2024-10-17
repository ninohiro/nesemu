#include "nes.h"

#define P_NZ (cpu.P=(~(128|2)&cpu.P)|(v1&128)|(2*(v1==0)))
#define P_C (cpu.P=(~1&cpu.P)|(1*((v2&v3)>>7==1||((v2|v3)>>7==1&&v1>>7==0))))
#define P_VC (cpu.P=(~(64|1)&cpu.P)|(64*((v2>>7==v3>>7)&&(v2>>7!=v1>>7)))|(1*((v2&v3)>>7==1||((v2|v3)>>7==1&&v1>>7==0))))

void NES::reset(){
    cpu.S-=3;
    unsigned short a1=(unsigned short)load_cpu_mem(0xFFFC)+(unsigned short)load_cpu_mem(0xFFFD)*256;
    cpu.PC=a1;
    cpu.P=32|4;
}

void NES::interrupt(unsigned short addr,bool b){
    cpu.S-=2;
    store_cpu_mem(0x100+(unsigned short)(cpu.S+1),(unsigned char)cpu.PC);
    store_cpu_mem(0x100+(unsigned short)(cpu.S+2),(unsigned char)(cpu.PC>>8));
    cpu.S--;
    store_cpu_mem(0x100+(unsigned short)(cpu.S+1),(~16&cpu.P)|(16*b));
    cpu.PC=addr;
    cpu.P|=4;
    cpu.wait=7;
}

unsigned char NES::load_cpu_mem(unsigned short addr){
    if(addr<=0x1FFF){
        addr&=0x7FF;
        return cpu.ram[addr];
    }
    else if(addr>=0x2000 && addr<=0x3FFF){
        addr&=0x7;
        unsigned char ret=ppu.registers[addr];
        switch(addr){
        case 0x2:
            ppu.write_toggle=false;
            ppu.registers[0x2]=~128&ppu.registers[0x2];
            break;
        case 0x4:
            ret=ppu.oam[ppu.registers[0x3]];
            break;
        case 0x7:
            ret=cpu.ppu_buf;
            cpu.ppu_buf=load_ppu_mem(ppu.internal_addr);
            if(ppu.internal_addr>=0x3F00){
                ret=cpu.ppu_buf;
            }
            ppu.internal_addr+=(ppu.registers[0x0]&4)?32:1;
            break;
        }
        return ret;
    }
    else if(addr>=0x4000 && addr<=0x401F){
        addr&=0x1F;
        if(addr==0x16){
            if(controller_index>=8){
                return 0;
            }
            return controller[controller_index++];
        }
        return io[addr];
    }
    else if(addr>=0x6000 && addr<=0x7FFF){
        addr&=0x1FFF;
        return ines.prg_ram[addr];
    }
    else if(addr>=0x8000){
        addr&=(ines.prg_size==1)?0x3FFF:0x7FFF;
        return ines.prg[addr];
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
        addr&=0x7;
        switch(addr){
        case 0x4:
            ppu.oam[ppu.registers[3]]=value;
            ppu.registers[3]+=1;
            break;
        case 0x5:
            if(!ppu.write_toggle){
                ppu.scroll_x=value;
            }
            else{
                ppu.scroll_y=value;
            }
            ppu.write_toggle=!ppu.write_toggle;
            break;
        case 0x6:
            if(!ppu.write_toggle){
                ppu.internal_addr=(unsigned short)value<<8;
                ppu.registers[0]=(~3&ppu.registers[0])|((value>>2)&3);
            }
            else{
                ppu.internal_addr|=value;
            }
            ppu.write_toggle=!ppu.write_toggle;
            break;
        case 0x7:
            store_ppu_mem(ppu.internal_addr,value);
            ppu.internal_addr+=(ppu.registers[0]&4)?32:1;
            break;
        default:
            ppu.registers[addr]=value;
        }
    }
    else if(addr>=0x4000 && addr<=0x401F){
        addr&=0x1F;
        if(addr==0x14){
            for(int i=0;i<256;i++){
                ppu.oam[(unsigned char)(ppu.registers[3]+i)]=load_cpu_mem(((unsigned short)value<<8)+i);
            }
            for(int i=0;i<33;i++){
                for(int j=0;j<33;j++){
                    ppu.oam_list_size[i][j]=0;
                }
            }
            for(int i=0;i<64;i++){
                int c_x=ppu.oam[i*4+3]/8,c_y=ppu.oam[i*4]/8;
                for(int j=0;j<4;j++){
                    int d_x=c_x+(j&1),d_y=c_y+(j>>1&1);
                    ppu.oam_list[d_y][d_x][ppu.oam_list_size[d_y][d_x]]=i;
                    ppu.oam_list_size[d_y][d_x]++;
                }
            }
            cpu.dma_wait=513;
        }
        else if(addr==0x16){
            if(!(value&1)){
                controller_index=0;
            }
        }
        else{
            io[addr]=value;
        }
    }
    else if(addr>=0x6000 && addr<=0x7FFF){
        addr&=0x1FFF;
        ines.prg_ram[addr]=value;
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
    if(pin_nmi){
        interrupt((unsigned short)load_cpu_mem(0xFFFA)|(unsigned short)load_cpu_mem(0xFFFB)<<8,0);
        pin_nmi=0;
        cpu.wait--;
        return;
    }
    unsigned short a1,a2;
    unsigned char v1,v2,v3;
    unsigned short s1;
    unsigned char c=load_cpu_mem(cpu.PC);
    switch(c&0x1F){
        case 0x0:
        case 0x2:
            if(c==0x00){
                cpu.PC+=2;
            }
            else if(c==0x20){
                a1=(unsigned short)load_cpu_mem(cpu.PC+1)|(unsigned short)load_cpu_mem(cpu.PC+2)<<8;
                cpu.PC+=3;
            }
            else if(c>=0x80){        //Immediate
                a1=cpu.PC+1;
                cpu.wait=2;
                cpu.PC+=2;
            }
            else{               //Implied
                cpu.wait=2;
                cpu.PC+=1;
            }
            break;
        case 0x1:
        case 0x3:               //(Indirect, X)
            a2=(unsigned char)(load_cpu_mem(cpu.PC+1)+cpu.X);
            a1=(unsigned short)load_cpu_mem(a2)|(unsigned short)load_cpu_mem((unsigned char)(a2+1))<<8;
            cpu.wait=6;
            cpu.PC+=2;
            break;
        case 0x4:
        case 0x5:
        case 0x6:
        case 0x7:               //Zeropage
            a1=load_cpu_mem(cpu.PC+1);
            cpu.wait=3;
            cpu.PC+=2;
            break;
        case 0x9:
        case 0xB:               //Immediate
            a1=cpu.PC+1;
            cpu.wait=2;
            cpu.PC+=2;
            break;
        case 0xC:
        case 0xD:
        case 0xE:
        case 0xF:               //Absolute
            a1=(unsigned short)load_cpu_mem(cpu.PC+1)|(unsigned short)load_cpu_mem(cpu.PC+2)<<8;
            if(c==0x6C){        //(Indirect)
                a1=(unsigned short)load_cpu_mem(a1)|(unsigned short)load_cpu_mem((a1&0xFF00)|((a1+1)&0xFF))<<8;
            }
            cpu.wait=4;
            cpu.PC+=3;
            break;
        case 0x10:              //Relative
            v1=load_cpu_mem(cpu.PC+1);
            a1=cpu.PC+2+(char)v1;
            cpu.wait=2+(((cpu.PC+2)&0xFF00)!=(a1&0xFF00));
            cpu.PC+=2;
            break;
        case 0x11:
        case 0x13:              //(Indirect), Y
            a2=load_cpu_mem(cpu.PC+1);
            v1=load_cpu_mem(a2);
            a1=((unsigned short)v1|(unsigned short)load_cpu_mem((unsigned char)(a2+1))<<8)+cpu.Y;
            cpu.wait=5+(c==0x91||((int)v1+(int)cpu.Y)>=256);
            cpu.PC+=2;
            break;
        case 0x14:
        case 0x15:
        case 0x16:
        case 0x17:              //Zeropage, X or Zeropage, Y
            a1=(unsigned char)(load_cpu_mem(cpu.PC+1)+((c==0x96||c==0x97||c==0xB6||c==0xB7)?cpu.Y:cpu.X));
            cpu.wait=4;
            cpu.PC+=2;
            break;
        case 0x19:
        case 0x1B:
        case 0x1C:
        case 0x1D:
        case 0x1E:
        case 0x1F:              //Absolute, X or Absolute, Y
            v1=load_cpu_mem(cpu.PC+1);
            v2=((c&0x1F)==0x19||(c&0x1F)==0x1B||c==0x9E||c==0x9F||c==0xBE||c==0xBF)?cpu.Y:cpu.X;
            a1=((unsigned short)v1|(unsigned short)load_cpu_mem(cpu.PC+2)<<8)+v2;
            cpu.wait=4+(c==0x9D||c==0x99||c==0x1E||c==0xDE||c==0xFE||c==0x5E||c==0x3E||c==0x7E||((int)v1+(int)v2)>=256?1:0);
            cpu.PC+=3;
            break;
        default:                //Implied
            cpu.wait=2;
            cpu.PC+=1;
    }
    switch(c){
    case 0xA9:
    case 0xA5:
    case 0xB5:
    case 0xAD:
    case 0xBD:
    case 0xB9:
    case 0xA1:
    case 0xB1:
        v1=cpu.A=load_cpu_mem(a1);
        P_NZ;
        break;
    case 0xA2:
    case 0xA6:
    case 0xB6:
    case 0xAE:
    case 0xBE:
        v1=cpu.X=load_cpu_mem(a1);
        P_NZ;
        break;
    case 0xA0:
    case 0xA4:
    case 0xB4:
    case 0xAC:
    case 0xBC:
        v1=cpu.Y=load_cpu_mem(a1);
        P_NZ;
        break;
    case 0x85:
    case 0x95:
    case 0x8D:
    case 0x9D:
    case 0x99:
    case 0x81:
    case 0x91:
        store_cpu_mem(a1,cpu.A);
        break;
    case 0x86:
    case 0x96:
    case 0x8E:
        store_cpu_mem(a1,cpu.X);
        break;
    case 0x84:
    case 0x94:
    case 0x8C:
        store_cpu_mem(a1,cpu.Y);
        break;
    case 0xAA:
        v1=cpu.X=cpu.A;
        P_NZ;
        break;
    case 0xA8:
        v1=cpu.Y=cpu.A;
        P_NZ;
        break;
    case 0xBA:
        v1=cpu.X=cpu.S;
        P_NZ;
        break;
    case 0x8A:
        v1=cpu.A=cpu.X;
        P_NZ;
        break;
    case 0x9A:
        v1=cpu.S=cpu.X;
        break;
    case 0x98:
        v1=cpu.A=cpu.Y;
        P_NZ;
        break;

    case 0x69:
    case 0x65:
    case 0x75:
    case 0x6D:
    case 0x7D:
    case 0x79:
    case 0x61:
    case 0x71:
        v2=cpu.A;
        v3=load_cpu_mem(a1);
        v1=cpu.A=v2+v3+(cpu.P&1);
        P_NZ;
        P_VC;
        break;

    case 0x29:
    case 0x25:
    case 0x35:
    case 0x2D:
    case 0x3D:
    case 0x39:
    case 0x21:
    case 0x31:
        v1=cpu.A&=load_cpu_mem(a1);
        P_NZ;
        break;

    case 0x0A:
        cpu.P=(~1&cpu.P)|(1*(cpu.A>>7));
        v1=cpu.A<<=1;
        P_NZ;
        break;
    case 0x06:
    case 0x16:
    case 0x0E:
    case 0x1E:
        v1=load_cpu_mem(a1);
        cpu.P=(~1&cpu.P)|(1*(v1>>7));
        v1<<=1;
        store_cpu_mem(a1,v1);
        P_NZ;
        cpu.wait+=2;
        break;

    case 0x24:
    case 0x2C:
        v1=load_cpu_mem(a1);
        cpu.P=(~(128|64|2)&cpu.P)|((128|64)&v1)|(2*((v1&cpu.A)==0));
        break;

    case 0xC9:
    case 0xC5:
    case 0xD5:
    case 0xCD:
    case 0xDD:
    case 0xD9:
    case 0xC1:
    case 0xD1:
        v2=cpu.A;
        goto CMP;
    case 0xE0:
    case 0xE4:
    case 0xEC:
        v2=cpu.X;
        goto CMP;
    case 0xC0:
    case 0xC4:
    case 0xCC:
        v2=cpu.Y;
    CMP:
        v3=~load_cpu_mem(a1);
        v1=v2+v3+1;
        P_NZ;
        P_C;
        break;
    case 0xC6:
    case 0xD6:
    case 0xCE:
    case 0xDE:
        v1=load_cpu_mem(a1);
        --v1;
        store_cpu_mem(a1,v1);
        P_NZ;
        cpu.wait+=2;
        break;
    case 0xCA:
        v1=--cpu.X;
        P_NZ;
        break;
    case 0x88:
        v1=--cpu.Y;
        P_NZ;
        break;

    case 0x49:
    case 0x45:
    case 0x55:
    case 0x4D:
    case 0x5D:
    case 0x59:
    case 0x41:
    case 0x51:
        v1=cpu.A^=load_cpu_mem(a1);
        P_NZ;
        break;

    case 0xE6:
    case 0xF6:
    case 0xEE:
    case 0xFE:
        v1=load_cpu_mem(a1);
        ++v1;
        store_cpu_mem(a1,v1);
        P_NZ;
        cpu.wait+=2;
        break;

    case 0xE8:
        v1=++cpu.X;
        P_NZ;
        break;
    case 0xC8:
        v1=++cpu.Y;
        P_NZ;
        break;

    case 0x4A:
        cpu.P=(~1&cpu.P)|(1*(cpu.A&1));
        v1=cpu.A>>=1;
        P_NZ;
        break;
    case 0x46:
    case 0x56:
    case 0x4E:
    case 0x5E:
        v1=load_cpu_mem(a1);
        cpu.P=(~1&cpu.P)|(1*(v1&1));
        v1>>=1;
        store_cpu_mem(a1,v1);
        P_NZ;
        cpu.wait+=2;
        break;

    case 0x09:
    case 0x05:
    case 0x15:
    case 0x0D:
    case 0x1D:
    case 0x19:
    case 0x01:
    case 0x11:
        v1=cpu.A|=load_cpu_mem(a1);
        P_NZ;
        break;

    case 0x2A:
        v2=cpu.P&1;
        cpu.P=(~1&cpu.P)|(1*(cpu.A>>7));
        cpu.A<<=1;
        v1=cpu.A|=v2;
        P_NZ;
        break;
    case 0x26:
    case 0x36:
    case 0x2E:
    case 0x3E:
        v1=load_cpu_mem(a1);
        v2=cpu.P&1;
        cpu.P=(~1&cpu.P)|(1*(v1>>7));
        v1<<=1;
        v1|=v2;
        store_cpu_mem(a1,v1);
        P_NZ;
        cpu.wait+=2;
        break;

    case 0x6A:
        v2=cpu.P&1;
        cpu.P=(~1&cpu.P)|(1*(cpu.A&1));
        cpu.A>>=1;
        v1=cpu.A|=v2<<7;
        P_NZ;
        break;
    case 0x66:
    case 0x76:
    case 0x6E:
    case 0x7E:
        v1=load_cpu_mem(a1);
        v2=cpu.P&1;
        cpu.P=(~1&cpu.P)|(1*(v1&1));
        v1>>=1;
        v1|=v2<<7;
        store_cpu_mem(a1,v1);
        P_NZ;
        cpu.wait+=2;
        break;

    case 0xE9:
    case 0xE5:
    case 0xF5:
    case 0xED:
    case 0xFD:
    case 0xF9:
    case 0xE1:
    case 0xF1:
        v2=cpu.A;
        v3=~load_cpu_mem(a1);
        v1=cpu.A=v2+v3+(cpu.P&1);
        P_NZ;
        P_VC;
        break;

    case 0x48:
        cpu.S--;
        store_cpu_mem(0x100+(unsigned char)(cpu.S+1),cpu.A);
        cpu.wait=3;
        break;
    case 0x08:
        cpu.S--;
        store_cpu_mem(0x100+(unsigned char)(cpu.S+1),cpu.P|16);
        cpu.wait=3;
        break;

    case 0x68:
        v1=cpu.A=load_cpu_mem(0x100+(unsigned char)(cpu.S+1));
        cpu.S++;
        P_NZ;
        cpu.wait=4;
        break;
    case 0x28:
        cpu.P=32|((~16)&load_cpu_mem(0x100+(unsigned char)(cpu.S+1)));
        cpu.S++;
        cpu.wait=4;
        break;

    case 0x4C:
    case 0x6C:
        cpu.PC=a1;
        if(c==0x4C){
            cpu.wait=3;
        }
        else{
            cpu.wait=5;
        }
        break;

    case 0x20:
        a2=cpu.PC-1;
        cpu.S-=2;
        store_cpu_mem(0x100+(unsigned char)(cpu.S+1),(unsigned char)a2);
        store_cpu_mem(0x100+(unsigned char)(cpu.S+2),(unsigned char)(a2>>8));
        cpu.PC=a1;
        cpu.wait=6;
        break;
    case 0x60:
        cpu.PC=(load_cpu_mem(0x100+(unsigned char)(cpu.S+1))|load_cpu_mem(0x100+(unsigned char)(cpu.S+2))<<8)+1;
        cpu.S+=2;
        cpu.wait=6;
        break;
    case 0x40:
        cpu.P=32|((~16)&load_cpu_mem(0x100+(unsigned char)(cpu.S+1)));
        cpu.S++;
        a1=load_cpu_mem(0x100+(unsigned char)(cpu.S+1))|load_cpu_mem(0x100+(unsigned char)(cpu.S+2))<<8;
        cpu.S+=2;
        cpu.PC=a1;
        cpu.wait=6;
        break;

    case 0x90:
        if(!(cpu.P&1)){
            cpu.PC=a1;
        }
        break;
    case 0xB0:
        if(cpu.P&1){
            cpu.PC=a1;
        }
        break;
    case 0xF0:
        if(cpu.P&2){
            cpu.PC=a1;
        }
        break;
    case 0x30:
        if(cpu.P&128){
            cpu.PC=a1;
        }
        break;
    case 0xD0:
        if(!(cpu.P&2)){
            cpu.PC=a1;
        }
        break;
    case 0x10:
        if(!(cpu.P&128)){
            cpu.PC=a1;
        }
        break;
    case 0x50:
        if(!(cpu.P&64)){
            cpu.PC=a1;
        }
        break;
    case 0x70:
        if(cpu.P&64){
            cpu.PC=a1;
        }
        break;

    case 0x18:
        cpu.P=(~(1)&cpu.P);
        break;
    case 0x58:
        cpu.P=(~(4)&cpu.P);
        break;
    case 0xB8:
        cpu.P=(~(64)&cpu.P);
        break;
    case 0x38:
        cpu.P=(~(1)&cpu.P)|1;
        break;
    case 0x78:
        cpu.P=(~(4)&cpu.P)|4;
        break;

    case 0x00:
        interrupt((unsigned short)load_cpu_mem(0xFFFE)|(unsigned short)load_cpu_mem(0xFFFF)<<8,1);
        break;
    case 0xF8:
        cpu.P|=8;
        break;
    case 0xD8:
        cpu.P=~8&cpu.P;
        break;
    case 0xEA:
        break;
    default:
        throw "Unknown instruction";
    }
    cpu.wait--;
}
