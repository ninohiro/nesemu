#include "ines.h"
#include <cstdlib>
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
    unsigned char pin_irq;
    unsigned char pin_nmi;
    unsigned char pin_reset;
    uint32_t *pixels;
public:
    NES(INES ines,uint32_t *pixels);
    void step_cpu();
    void step_ppu();
    unsigned char load_cpu_mem(unsigned short addr);
    void store_cpu_mem(unsigned short addr,unsigned char value);
};
