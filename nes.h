#include <SDL2/SDL.h>
#include "ines.h"
struct CPU{
    unsigned char ram[2048];
    unsigned char A;
    unsigned char X;
    unsigned char Y;
    unsigned char S;
    unsigned short PC;
    unsigned char P;
    int wait;
};
struct PPU{
    unsigned char registers[8];
};
class NES{
    CPU cpu;
    PPU ppu;
    INES ines;
    unsigned char io[32];
    Uint32 *pixels;
public:
    NES(INES ines,Uint32 *pixels);
    void step_cpu();
    void step_ppu();
    unsigned char load_cpu_mem(unsigned short addr);
    void store_cpu_mem(unsigned short addr,unsigned char value);
};
