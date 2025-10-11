/*

    This is my own custom CPU design. It's a 16 bit CPU with a 32 bit address bus.
    You can define the memory map yourself and write your own programs in a separate
    file. (not implemented yet)

*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "instruction_set.c"

struct data {
    uint8_t C : 1; // Carry
	uint8_t Z : 1; // Zero
	uint8_t I : 1; // Interrupt disable
	uint8_t D : 1; // Decimal mode
	uint8_t B : 1; // Break command
	uint8_t V : 1; // Overflow flag
	uint8_t N : 1; // Negative flag
    uint8_t clk : 1; // Clock going (set to 0 to stop the program)

    uint32_t PC;

    uint16_t registers[16];
    uint16_t SP;

    uint16_t exit_code;
    uint32_t cycle_num;
};

