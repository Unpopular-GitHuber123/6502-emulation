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
const uint16_t RAM_RANGE[2] = {0x0200, 0x7FFF};
const uint16_t ROM_RANGE[2] = {0x8000, 0xFFFF};
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

void stackPush(struct data *data, uint8_t *mem, uint8_t val, uint8_t testing_mode) {
	mem[data -> SP + STACK_RANGE[0]] = val;

	data -> SP--;

	if (testing_mode > 2) {
		printf("Value pushed to stack: %02x\n", val);
	}

	return;
}

uint8_t stackPop(struct data *data, uint8_t *mem, uint8_t testing_mode) {
	data -> SP++;
	uint8_t toReturn = mem[data -> SP + STACK_RANGE[0]];

	if (testing_mode > 2) {
		printf("Value returned from stack: %02x\n", toReturn);
	}

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

uint16_t execute(struct data *data, uint8_t *mem, uint16_t *address, uint8_t testing_mode) {
	if (testing_mode > 0) {
		printf("Instruction: %02x\n", mem[*address]);
		printf("Address: %04x\n", *address);
	}
	switch (mem[*address]) {
		case INS_BRK_IP:
			data -> clk = 0;
			break;
		case INS_AND_IM:
			(*address)++;
			data -> A = (data -> A & mem[*address]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			break;
		case INS_AND_ZP:
			(*address)++;
			data -> A = (data -> A & mem[mem[*address]]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			break;
		case INS_AND_ZX:
			(*address)++;
			data -> A = (data -> A & (uint8_t) (mem[mem[*address] + data -> X]));
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			break;
		case INS_AND_AB:
			(*address)++;
			data -> A = (data -> A & (uint8_t) mem[getWord(data, address, mem)]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			break;
		case INS_AND_AX:
			(*address)++;
			data -> A = (data -> A & (uint8_t) mem[getWord(data, address, mem) + data -> X]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			break;
		case INS_AND_AY:
			(*address)++;
			data -> A = (data -> A & (uint8_t) mem[getWord(data, address, mem) + data -> Y]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			break;
		case INS_AND_IX:
			(*address)++;
			uint16_t temp = (uint8_t) (mem[*address] + data -> X);
			data -> A = (data -> A & mem[getWord(data, &temp, mem)]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			break;
		case INS_AND_IY:
			(*address)++;
			temp = (uint8_t) (mem[*address] + data -> Y);
			data -> A = (data -> A & mem[getWord(data, &temp, mem)]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			break;
		case INS_ADC_IM:
			(*address)++;
			uint16_t output = data -> A + mem[*address];
			if (data -> C == 1) { output += 256; }
			data -> C = (output >= 256);
			data -> V = ((data -> A & 0b10000000) != (output & 0b10000000));
			data -> A = output;
			data -> N = (data -> A & 0b10000000 > 1);
			data -> Z = (data -> A == 0);
			if (testing_mode > 1) {
				printf("accumulator: %02x\n", data -> A);
			}
			break;
		case INS_ADC_ZP:
			(*address)++;
			output = data -> A + mem[mem[*address]];
			if (data -> C == 1) { output += 256; }
			data -> C = (output >= 256);
			data -> V = ((data -> A & 0b10000000) != (output & 0b10000000));
			data -> A = output;
			data -> N = (data -> A & 0b10000000 > 1);
			data -> Z = (data -> A == 0);
			if (testing_mode > 1) {
				printf("accumulator: %02x\n", data -> A);
			}
			break;
		case INS_ADC_ZX:
			(*address)++;
			output = data -> A + (uint8_t) (mem[mem[*address] + data -> X]);
			if (data -> C == 1) { output += 256; }
			data -> C = (output >= 256);
			data -> V = ((data -> A & 0b10000000) != (output & 0b10000000));
			data -> A = output;
			data -> N = (data -> A & 0b10000000 > 1);
			data -> Z = (data -> A == 0);
			if (testing_mode > 1) {
				printf("accumulator: %02x\n", data -> A);
			}
			break;
		case INS_ADC_AB:
			(*address)++;
			output = data -> A + mem[getWord(data, address, mem)];
			if (data -> C == 1) { output += 256; }
			data -> C = (output >= 256);
			data -> V = ((data -> A & 0b10000000) != (output & 0b10000000));
			data -> A = output;
			data -> N = (data -> A & 0b10000000 > 1);
			data -> Z = (data -> A == 0);
			if (testing_mode > 1) {
				printf("accumulator: %02x\n", data -> A);
			}
			break;
		case INS_ADC_AX:
			(*address)++;
			uint16_t tempAdr = (*address) + data -> X;
			output = data -> A + mem[getWord(data, &tempAdr, mem)];
			if (data -> C == 1) { output += 256; }
			data -> C = (output >= 256);
			data -> V = ((data -> A & 0b10000000) != (output & 0b10000000));
			data -> A = output;
			data -> N = (data -> A & 0b10000000 > 1);
			data -> Z = (data -> A == 0);
			if (testing_mode > 1) {
				printf("accumulator: %02x\n", data -> A);
			}
			break;
		case INS_ADC_AY:
			(*address)++;
			tempAdr = (*address) + data -> Y;
			output = data -> A + mem[getWord(data, &tempAdr, mem)];
			if (data -> C == 1) { output += 256; }
			data -> C = (output >= 256);
			data -> V = ((data -> A & 0b10000000) != (output & 0b10000000));
			data -> A = output;
			data -> N = (data -> A & 0b10000000 > 1);
			data -> Z = (data -> A == 0);
			if (testing_mode > 1) {
				printf("accumulator: %02x\n", data -> A);
			}
			break;
		case INS_ADC_IX:
			(*address)++;
			temp = (uint8_t) (data -> X + mem[*address]);
			output = data -> A + mem[getWord(data, &temp, mem)];
			if (data -> C == 1) { output += 256; }
			data -> C = (output >= 256);
			data -> V = ((data -> A & 0b10000000) != (output & 0b10000000));
			data -> A = output;
			data -> N = (data -> A & 0b10000000 > 1);
			data -> Z = (data -> A == 0);
			if (testing_mode > 1) {
				printf("accumulator: %02x\n", data -> A);
			}
			break;
		case INS_ADC_IY:
			(*address)++;
			temp = mem[*address];
			output = data -> A + mem[(uint8_t) getWord(data, &temp, mem) + data -> Y];
			if (data -> C == 1) { output += 256; }
			data -> C = (output >= 256);
			data -> V = ((data -> A & 0b10000000) != (output & 0b10000000));
			data -> A = output;
			data -> N = (data -> A & 0b10000000 > 1);
			data -> Z = (data -> A == 0);
			if (testing_mode > 1) {
				printf("accumulator: %02x\n", data -> A);
			}
			break;
		case INS_JMP_AB:
			(*address)++;
			*address = getWord(data, address, mem) - 1;
			if (testing_mode > 1) {
				printf("Address jumped to: %04x\n", *address + 1);
			}
			break;
		case INS_JMP_ID:
			(*address)++;
			uint16_t addr = getWord(data, address, mem);
			(*address) = getWord(data, &addr, mem) - 1;
			if (testing_mode > 1) {
				printf("Address jumped to: %04x\n", *address + 1);
			}
			break;
		case INS_JSR_AB:
			(*address)++;
			stackPush(data, mem, (*address) - 1, testing_mode);
			stackPush(data, mem, ((*address) - 1) >> 8, testing_mode);
			*address = getWord(data, address, mem) - 1;
			if (testing_mode > 1) {
				printf("Address jumped to: %04x\n", *address + 1);
			}
			break;
		case INS_RTS_IP:
			uint8_t highByte = stackPop(data, mem, testing_mode);
			uint8_t lowByte = stackPop(data, mem, testing_mode);
			(*address) = (lowByte | (highByte << 8));
			if (testing_mode > 3) {
				printf("Low address byte: %02x\n", lowByte);
				printf("High address byte: %02x\n", highByte);
			}
			if (testing_mode > 1) {
				printf("Address returned to: %04x\n", *address + 1);
			}
			break;
		case INS_LDX_IM:
			(*address)++;
			data -> X = mem[*address];
			break;
		case INS_LDY_IM:
			(*address)++;
			data -> Y = mem[*address];
			break;
		case INS_LDA_IM:
			(*address)++;
			data -> A = mem[*address];
			break;
		case INS_CLD_IP:
			data -> D = 0;
			break;
		case INS_SED_IP:
			data -> D = 1;
			break;
		case INS_CLC_IP:
			data -> C = 0;
			break;
		case INS_SEC_IP:
			data -> C = 1;
			break;
		case INS_CLI_IP:
			data -> I = 0;
			break;
		case INS_SEI_IP:
			data -> I = 1;
			break;
		case INS_CLV_IP:
			data -> V = 0;
			break;
		case INS_NOP_IP:
			break;
		default:
			printf("Unrecognised instruction at address: %04x\n", *address);
	}
	if (testing_mode > 3) {
		printf("C: %d Z: %d I: %d D: %d B: %d clk: %d V: %d N: %d\n", data -> C, data -> Z, data -> I, data -> D, data -> B, data -> clk, data -> V, data -> N);
		printf("PC: %04x\n", data -> PC);
		printf("A: %02x\n", data -> A);
		printf("X: %02x\n", data -> X);
		printf("Y: %02x\n", data -> Y);
		printf("SP: %02x\n", data -> SP);
	}
	if (testing_mode > 0) {
		printf("\n");
	}
	(*address)++;
	return 0x0002;
}
