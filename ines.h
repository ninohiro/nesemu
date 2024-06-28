#pragma once

struct INES{
    unsigned char header[16];
    unsigned char prg_size;
    unsigned char chr_size;
    unsigned char flag;
    unsigned char prg[32768];
    unsigned char chr[8192];
    unsigned char prg_ram[8192];
    unsigned char chr_ram[2048];
};
INES read_rom(const char *s);
