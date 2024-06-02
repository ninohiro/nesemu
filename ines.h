#include <cstdlib>
#include <vector>
#include <string>
struct INES{
    unsigned char prg_size;
    unsigned char chr_size;
    std::vector<unsigned char> prg;
    std::vector<unsigned char> chr;
    unsigned char flag;
};
INES read_rom(const std::string &s);
