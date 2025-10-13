#include "cpucustom.c"

int main() {
    int testing_mode = 4;
    struct data data;
    uint8_t *mem = (uint8_t*) malloc(64 * 1024 * 1024); // 64 megs. Most addressable memory is 4 gigs!

    uint32_t ranges[] = 
    {
        0x4000000, // Total mem
        0x0000000, // Stack start
        0x000FFFF, // Stack end
        0x0010000, // Main RAM start
        0x1FFFFFF, // Main RAM end
        0x2000000, // Program memory start
        0x4000000  // Program memory end
    };

    if (mem == NULL) {
        printf("err: malloc failed\n");
        return 1;
    }

    while (data.clk == 1) {
        execute(&data, mem, testing_mode, ranges);
        break;
    }

    free(mem);
    return 0;
}