#include "ines.h"
#include <cstdlib>
struct CPU{
    unsigned char ram[2048];
    unsigned char prg_ram[8192];
    unsigned char A;
    unsigned char X;
    unsigned char Y;
    unsigned char S;
    unsigned short PC;
    unsigned char P;
    int wait;
    int dma_wait;
    unsigned char prev_instr;
    unsigned short prev_pc;
};
struct PPU{
    unsigned char registers[8];
    unsigned char vram[2048];
    unsigned char palette_ram[32];
    unsigned char read_buf;
    int counter;
    bool odd_even;
    bool write_toggle;
    unsigned char scroll_x;
    unsigned char scroll_y;
    unsigned short internal_addr;
    unsigned char oam[256];
    unsigned char oam_list[33][33][64];
    unsigned char oam_list_size[33][33];
    unsigned char chr_ram[8192];
};
struct NES{
    CPU cpu;
    PPU ppu;
    INES ines;
    unsigned char io[32];
    unsigned char pin_irq;
    unsigned char pin_nmi;
    unsigned char pin_reset;
    unsigned char controller[8];
    int controller_index;
    bool polling;
    uint32_t *pixels;
    void step_cpu();
    void step_ppu();
    unsigned char load_cpu_mem(unsigned short addr);
    unsigned char load_ppu_mem(unsigned short addr);
    void store_cpu_mem(unsigned short addr,unsigned char value);
    void store_ppu_mem(unsigned short addr,unsigned char value);
    void render();
    void reset();
};
