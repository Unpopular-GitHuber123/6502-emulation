/*******************************************************

This emulation is similar to the MOS 6502 processor.
It is not a 1:1 emulation, especially during the boot
sequence, but it is close-ish.

It has a maximum addressable memory of 64 KB, including
RAM, ROM, and a stack. These would be external
components in an actual physical circuit.

Credits:
Code: NoWayAmIAGuest/Unpopular-GitHuber123
Helpful video: https://www.youtube.com/watch?v=qJgsuQoy9bc
(Very) helpful website: http://www.6502.org/users/obelisk/index.html

*******************************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "instruction_set.c"

// Memory (Change the ranges as you want but be prepared for seg faults and unexpected behaviour):
const uint16_t MAX_MEM = 0xFFFF;
const uint16_t ZPAGE_RANGE[2] = {0x0000, 0x00FF};
const uint16_t STACK_RANGE[2] = {0x0100, 0x01FF};
const uint16_t RAM_RANGE[2] = {0x0200, 0x55FF};
const uint16_t ROM_RANGE[2] = {0x5600, 0xFFFF};
const uint16_t SYSMEM_RANGE[2] = {0xFF00, 0xFFFF};

struct data {
	uint8_t C : 1; // Carry
	uint8_t Z : 1; // Zero
	uint8_t I : 1; // Interrupt disable
	uint8_t D : 1; // Decimal mode
	uint8_t B : 1; // Break command
	uint8_t clk : 1; // Clock going (set to 0 to stop the program)
	uint8_t V : 1; // Overflow flag
	uint8_t N : 1; // Negative flag

	uint16_t PC; // Program counter
	uint8_t SP; // Stack pointer
    
    uint8_t exit_code;
	uint8_t A, X, Y; // Accumulator, X, Y

	uint32_t cyclenum;
};

int getPS(struct data data) {
	int PS = 0;

	if (data.C) {
		PS += 1;
	}
	if (data.Z) {
		PS += 2;
	}
	if (data.I) {
		PS += 4;
	}
	if (data.D) {
		PS += 8;
	}
	if (data.B) {
		PS += 16;
	}
	if (data.clk) {
		PS += 32;
	}
	if (data.V) {
		PS += 64;
	}
	if (data.N) {
		PS += 128;
	}

	return PS;
}

void stackPush(struct data *data, uint8_t *mem, uint8_t val) {
	mem[data -> SP + STACK_RANGE[0]] = val;

	data -> SP--;
	return;
}

uint8_t stackPop(struct data *data, uint8_t *mem) {
	uint8_t toReturn = mem[data -> SP + STACK_RANGE[0]];

	data -> SP++;
	return toReturn;
}

void storeMem(uint8_t *mem, uint16_t address, uint8_t value, struct data *data) {
	if (address >= RAM_RANGE[0] && address <= RAM_RANGE[1]) {
		mem[address] = value;
	} else {
		data -> exit_code = 1;
	}
	return;
}

void setFlagsLDA(struct data *data, uint8_t val) {
	data -> A = val;
	data -> Z = (data -> A == 0);
	data -> N = (data -> A & 0b10000000) > 0;
	return;
}

uint16_t getWord(struct data *data, uint16_t *address, uint8_t *mem) {
	uint8_t lowByte = mem[*address];
	(*address)++;
	uint8_t highByte = mem[*address];
	(*address)++;
	return (lowByte | (highByte << 8));
}

uint8_t* initialise_mem(struct data data, uint8_t* mem) {
	for (uint32_t i = RAM_RANGE[0]; i < RAM_RANGE[1]; i++) {
		mem[i] = 0;
	}

	return mem;
}

struct data reset(struct data data) {
	data.clk = 1;
	data.PC = 0xFFFC;
	data.cyclenum = 0;
	data.exit_code = 0;

	return data;
}

uint16_t execute(struct data *data, uint8_t *mem, uint16_t *address, uint8_t instruction, uint8_t testing_mode) {
	data -> cyclenum += cycles[instruction];
	if (instruction == 0x0000) {
		instruction = mem[*address];
		return instruction;
	}


	printf("Instruction: %02x\n", instruction);
	printf("Address: %04x\n", *address);
	switch (instruction) {
		case INS_BRK_IP:
			data -> clk = 0;
			break;
		case INS_JMP_AB:
			uint16_t addr = getWord(data, address, mem);
			*address = addr;
			break;
		case INS_CLD_IP:
			data -> D = 0;
			break;
		case INS_SED_IP:
			data -> D = 1;
			break;
		default:
			printf("Unrecognised instruction at address: %04x", *address);
	}

	return 0x0000;
}
