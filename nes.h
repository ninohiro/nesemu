#pragma once

#include <cstdint>

#include "ines.h"

struct CPU{
    unsigned char ram[2048];
    unsigned char A;
    unsigned char X;
    unsigned char Y;
    unsigned char S;
    unsigned short PC;
    unsigned char P;
    unsigned char ppu_buf;
    int wait;
    int dma_wait;
};
struct PPU{
    unsigned char registers[8];
    unsigned char vram[2048];
    unsigned char palette_ram[32];
    int counter;
    bool odd_even;
    bool write_toggle;
    unsigned char scroll_x;
    unsigned char scroll_y;
    unsigned short internal_addr;
    unsigned char oam[256];
    unsigned char oam_list[33][33][64];
    unsigned char oam_list_size[33][33];
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
    std::uint32_t *pixels;
    void step_cpu();
    void step_ppu();
    int load_chr(int bank,int n,int c_x,int c_y);
    unsigned char load_cpu_mem(unsigned short addr);
    unsigned char load_ppu_mem(unsigned short addr);
    void store_cpu_mem(unsigned short addr,unsigned char value);
    void store_ppu_mem(unsigned short addr,unsigned char value);
    void render();
    void reset();
    void interrupt(unsigned short addr,bool b);
};
