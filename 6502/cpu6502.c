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

uint8_t getPS(struct data data) {
	uint8_t PS = 0;

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

void setPS(struct data *data, uint8_t PS) {
	data -> C = (PS & 0b00000001) > 1;
	data -> Z = (PS & 0b00000010) > 1;
	data -> I = (PS & 0b00000100) > 1;
	data -> D = (PS & 0b00001000) > 1;
	data -> B = (PS & 0b00010000) > 1;
	data -> V = (PS & 0b01000000) > 1;
	data -> N = (PS & 0b10000000) > 1;

	return;
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
		case MTA_OFF_IP:
			data -> clk = 0;
			break;
		case INS_BRK_IP:
			uint16_t temp = 0xFFFE;
			stackPush(data, mem, data -> PC, testing_mode);
			stackPush(data, mem, getPS(*data), testing_mode);
			data -> B = 1;
			data -> PC = getWord(data, &temp, mem);
			break;
		case INS_STA_ZP:
			(*address)++;
			mem[mem[*address]] = data -> A;
			break;
		case INS_STA_ZX:
			(*address)++;
			mem[(mem[*address] + data -> X) & 0b11111111] = data -> A;
			break;
		case INS_STA_AB:
			(*address)++;
			mem[getWord(data, address, mem)] = data -> A;
			break;
		case INS_STA_AX:
			(*address)++;
			mem[getWord(data, address, mem) + data -> X] = data -> A;
			break;
		case INS_STA_AY:
			(*address)++;
			mem[getWord(data, address, mem) + data -> Y] = data -> A;
			break;
		case INS_STA_IX:
			(*address)++;
			mem[(mem[(*address) + data -> X]) & 0b11111111] = data -> A;
			break;
		case INS_STA_IY:
			(*address)++;
			mem[mem[*address] + data -> Y] = data -> A;
			break;
		case INS_RTI_IP:
			setPS(data, stackPop(data, mem, testing_mode));
			data -> PC = stackPop(data, mem, testing_mode);
			break;
		case INS_STX_ZP:
			(*address)++;
			mem[mem[*address]] = data -> X;
			break;
		case INS_STX_ZY:
			(*address)++;
			mem[(mem[*address] + data -> Y) & 0b11111111] = data -> X;
			break;
		case INS_STX_AB:
			(*address)++;
			mem[getWord(data, address, mem)] = data -> X;
			break;
		case INS_STY_AB:
			(*address)++;
			mem[getWord(data, address, mem)] = data -> Y;
			break;
		case INS_STY_ZP:
			(*address)++;
			mem[mem[*address]] = data -> Y;
			break;
		case INS_STY_ZX:
			(*address)++;
			mem[(mem[*address] + data -> X) & 0b11111111] = data -> Y;
			break;
		case INS_TAX_IP:
			data -> X = data -> A;
			data -> Z = (data -> X == 0);
			data -> N = ((data -> X & 0b10000000) > 0);
			break;
		case INS_TAY_IP:
			data -> Y = data -> A;
			data -> Z = (data -> Y == 0);
			data -> N = ((data -> Y & 0b10000000) > 0);
			break;
		case INS_TYA_IP:
			data -> A = data -> Y;
			data -> Z = (data -> A == 0);
			data -> N = ((data -> A & 0b10000000) > 0);
			break;
		case INS_TXA_IP:
			data -> A = data -> X;
			data -> Z = (data -> A == 0);
			data -> N = ((data -> A & 0b10000000) > 0);
			break;
		case INS_TSX_IP:
			data -> X = *address;
			data -> Z = (data -> X == 0);
			data -> N = ((data -> X & 0b10000000) > 0);
			break;
		case INS_TXS_IP:
			*address = data -> X;
			break;
		case INS_DEC_ZP:
			(*address)++;
			mem[mem[*address]]--;
			break;
		case INS_DEC_ZX:
			(*address)++;
			mem[(mem[*address] + data -> X) & 0b11111111]--;
			break;
		case INS_DEC_AB:
			(*address)++;
			mem[getWord(data, address, mem)]--;
			break;
		case INS_DEC_AX:
			(*address)++;
			mem[getWord(data, address, mem) + data -> X]--;
			break;
		case INS_INC_ZP:
			(*address)++;
			mem[mem[*address]]++;
			break;
		case INS_INC_ZX:
			(*address)++;
			mem[(mem[*address] + data -> X) & 0b11111111]++;
			break;
		case INS_INC_AB:
			(*address)++;
			mem[getWord(data, address, mem)]++;
			break;
		case INS_INC_AX:
			(*address)++;
			mem[getWord(data, address, mem) + data -> X]++;
			break;
		case INS_DEX_IP:
			data -> X--;
			data -> Z = (data -> X == 0);
			data -> B = (data -> X & 0b10000000 > 1);
			break;
		case INS_INX_IP:
			data -> X++;
			data -> Z = (data -> X == 0);
			data -> B = (data -> X & 0b10000000 > 1);
			break;
		case INS_DEY_IP:
			data -> Y--;
			data -> Z = (data -> Y == 0);
			data -> B = (data -> Y & 0b10000000 > 1);
			break;
		case INS_INY_IP:
			data -> Y++;
			data -> Z = (data -> Y == 0);
			data -> B = (data -> Y & 0b10000000 > 1);
			break;
		case INS_ROL_AC:
			temp = (data -> A & 0b10000000);
			data -> A << 1;
			data -> A += data -> C;
			data -> C = temp;
			data -> Z = (data -> A == 0);
			break;
		case INS_ROL_ZP:
			(*address)++;
			temp = (mem[mem[*address]] & 0b10000000);
			mem[mem[*address]] << 1;
			mem[mem[*address]] += data -> C;
			data -> C = temp;
			data -> Z = (mem[mem[*address]] & 0b10000000 == 0);
			break;
		case INS_ROL_ZX:
			(*address)++;
			temp = (mem[(mem[*address] + data -> X) & 0b11111111] & 0b10000000);
			mem[(mem[*address] + data -> X) & 0b11111111] << 1;
			mem[(mem[*address] + data -> X) & 0b11111111] += data -> C;
			data -> C = temp;
			data -> Z = (mem[(mem[*address] + data -> X) & 0b11111111] == 0);
			break;
		case INS_ROL_AB:
			(*address)++;
			uint16_t temp2 = getWord(data, address, mem);
			temp = (mem[temp2] & 0b10000000);
			mem[temp2] << 1;
			mem[temp2] += data -> C;
			data -> C = temp;
			data -> Z = (mem[temp2] == 0);
			break;
		case INS_ROL_AX:
			(*address)++;
			temp2 = getWord(data, address, mem) + data -> X;
			temp = (mem[temp2] & 0b10000000);
			mem[temp2] << 1;
			mem[temp2] += data -> C;
			data -> C = temp;
			data -> Z = (mem[temp2] == 0);
			break;
		case INS_ROR_AC:
			temp = (data -> A & 0b10000000);
			data -> A >> 1;
			data -> A += data -> C;
			data -> C = temp;
			data -> Z = (data -> A == 0);
			break;
		case INS_ROR_ZP:
			(*address)++;
			temp = (mem[mem[*address]] & 0b10000000);
			mem[mem[*address]] >> 1;
			mem[mem[*address]] += data -> C;
			data -> C = temp;
			data -> Z = (mem[mem[*address]] & 0b10000000 == 0);
			break;
		case INS_ROR_ZX:
			(*address)++;
			temp = (mem[(mem[*address] + data -> X) & 0b11111111] & 0b10000000);
			mem[(mem[*address] + data -> X) & 0b11111111] >> 1;
			mem[(mem[*address] + data -> X) & 0b11111111] += data -> C;
			data -> C = temp;
			data -> Z = (mem[(mem[*address] + data -> X) & 0b11111111] == 0);
			break;
		case INS_ROR_AB:
			(*address)++;
			temp2 = getWord(data, address, mem);
			temp = (mem[temp2] & 0b10000000);
			mem[temp2] >> 1;
			mem[temp2] += data -> C;
			data -> C = temp;
			data -> Z = (mem[temp2] == 0);
			break;
		case INS_ROR_AX:
			(*address)++;
			temp2 = getWord(data, address, mem) + data -> X;
			temp = (mem[temp2] & 0b10000000);
			mem[temp2] >> 1;
			mem[temp2] += data -> C;
			data -> C = temp;
			data -> Z = (mem[temp2] == 0);
			break;
		case INS_ASL_AC:
			data -> C = ((data -> A & 0b10000000) > 0);
			data -> A << 1;
			data -> Z = (data -> A == 0);
			data -> N = ((data -> A & 0b10000000) > 0);
			break;
		case INS_ASL_ZP:
			uint16_t *temp1 = (uint16_t*) &(mem[mem[*address]]);
			(*address)++;
			data -> C = ((*temp1 & 0b10000000) > 0);
			*temp1 << 1;
			data -> Z = (*temp1 == 0);
			data -> N = ((*temp1 & 0b10000000) > 0);
			break;
		case INS_ASL_ZX:
			(*address)++;
			temp1 = (uint16_t*) (uint8_t*) &(mem[mem[*address] + data -> X]);
			data -> C = ((*temp1 & 0b10000000) > 0);
			*temp1 << 1;
			data -> Z = (*temp1 == 0);
			data -> N = ((*temp1 & 0b10000000) > 0);
			break;
		case INS_ASL_AB:
			(*address)++;
			temp1 = (uint16_t*) &(mem[getWord(data, address, mem)]);
			data -> C = ((*temp1 & 0b10000000) > 0);
			*temp1 << 1;
			data -> Z = (*temp1 == 0);
			data -> N = ((*temp1 & 0b10000000) > 0);
			break;
		case INS_ASL_AX:
			(*address)++;
			temp1 = (uint16_t*) &(mem[getWord(data, address, mem) + data -> X]);
			data -> C = ((*temp1 & 0b10000000) > 0);
			*temp1 << 1;
			data -> Z = (*temp1 == 0);
			data -> N = ((*temp1 & 0b10000000) > 0);
			break;
		case INS_LSR_AC:
			data -> C = ((*temp1 & 0b00000001) > 0);
			data -> A >> 1;
			data -> Z = (*temp1 == 0);
			data -> N = 0;
			break;
		case INS_LSR_ZP:
			temp1 = (uint16_t*) &(mem[mem[*address]]);
			(*address)++;
			data -> C = ((*temp1 & 0b00000001) > 0);
			*temp1 >> 1;
			data -> Z = (*temp1 == 0);
			data -> N = 0;
			break;
		case INS_LSR_ZX:
			(*address)++;
			temp1 = (uint16_t*) (uint8_t*) &(mem[mem[*address] + data -> X]);
			data -> C = ((*temp1 & 0b00000001) > 0);
			*temp1 >> 1;
			data -> Z = (*temp1 == 0);
			data -> N = 0;
			break;
		case INS_LSR_AB:
			(*address)++;
			temp1 = (uint16_t*) &(mem[getWord(data, address, mem)]);
			data -> C = ((*temp1 & 0b00000001) > 0);
			*temp1 >> 1;
			data -> Z = (*temp1 == 0);
			data -> N = 0;
			break;
		case INS_LSR_AX:
			(*address)++;
			temp1 = (uint16_t*) &(mem[getWord(data, address, mem) + data -> X]);
			data -> C = ((*temp1 & 0b00000001) > 0);
			*temp1 >> 1;
			data -> Z = (*temp1 == 0);
			data -> N = 0;
			break;
		case INS_CMP_IM:
			(*address)++;
			temp = (uint8_t) data -> A - mem[*address];
			data -> N = (temp & 0b10000000 > 0);
			data -> C = (data -> A >= mem[*address]);
			data -> Z = (data -> A > mem[*address]);
			break;
		case INS_CMP_ZP:
			(*address)++;
			temp = (uint8_t) data -> A - mem[mem[*address]];
			data -> N = (temp & 0b10000000 > 0);
			data -> C = (data -> A >= mem[*address]);
			data -> Z = (data -> A > mem[*address]);
			break;
		case INS_CMP_ZX:
			(*address)++;
			temp = (uint8_t) data -> A - mem[(mem[*address] + data -> X) & 0b11111111];
			data -> N = (temp & 0b10000000 > 0);
			data -> C = (data -> A >= mem[*address]);
			data -> Z = (data -> A > mem[*address]);
			break;
		case INS_CMP_AB:
			(*address)++;
			temp = (uint8_t) data -> A - mem[getWord(data, address, mem)];
			data -> N = (temp & 0b10000000 > 0);
			data -> C = (data -> A >= mem[*address]);
			data -> Z = (data -> A > mem[*address]);
			break;
		case INS_CMP_AX:
			(*address)++;
			temp = (uint8_t) data -> A - mem[(getWord(data, address, mem) + data -> X) & 0b11111111];
			data -> N = (temp & 0b10000000 > 0);
			data -> C = (data -> A >= mem[*address]);
			data -> Z = (data -> A > mem[*address]);
			break;
		case INS_CMP_AY:
			(*address)++;
			temp = (uint8_t) data -> A - mem[(getWord(data, address, mem) + data -> Y) & 0b11111111];
			data -> N = (temp & 0b10000000 > 0);
			data -> C = (data -> A >= mem[*address]);
			data -> Z = (data -> A > mem[*address]);
			break;
		case INS_CMP_IX:
			(*address)++;
			uint8_t temp3 = mem[(mem[*address] + data -> X) & 0b11111111];
			temp = (data -> A - mem[temp3]) & 0b11111111;
			data -> N = (temp & 0b10000000 > 0);
			data -> C = (data -> A >= mem[*address]);
			data -> Z = (data -> A > mem[*address]);
			break;
		case INS_CMP_IY:
			(*address)++;
			temp3 = mem[mem[*address]];
			temp = (data -> A - ((getWord(data, (uint16_t*) (&temp3), mem) + data -> Y) & 0b11111111)) & 0b11111111;
			data -> N = (temp & 0b10000000 > 0);
			data -> C = (data -> A >= mem[*address]);
			data -> Z = (data -> A > mem[*address]);
			break;
		case INS_CPX_IM:
			(*address)++;
			temp = (uint8_t) data -> X - mem[*address];
			data -> N = (temp & 0b10000000 > 0);
			data -> C = (data -> X >= mem[*address]);
			data -> Z = (data -> X > mem[*address]);
			break;
		case INS_CPX_ZP:
			(*address)++;
			temp = (uint8_t) data -> X - mem[mem[*address]];
			data -> N = (temp & 0b10000000 > 0);
			data -> C = (data -> X >= mem[*address]);
			data -> Z = (data -> X > mem[*address]);
			break;
		case INS_CPX_AB:
			(*address)++;
			temp = (uint8_t) data -> X - mem[*address];
			data -> N = (temp & 0b10000000 > 0);
			data -> C = (data -> X >= mem[*address]);
			data -> Z = (data -> X > mem[*address]);
			break;
		case INS_CPY_IM:
			(*address)++;
			temp = (uint8_t) data -> Y - mem[*address];
			data -> N = (temp & 0b10000000 > 0);
			data -> C = (data -> Y >= mem[*address]);
			data -> Z = (data -> Y > mem[*address]);
			break;
		case INS_CPY_ZP:
			(*address)++;
			temp = (uint8_t) data -> Y - mem[mem[*address]];
			data -> N = (temp & 0b10000000 > 0);
			data -> C = (data -> Y >= mem[*address]);
			data -> Z = (data -> Y > mem[*address]);
			break;
		case INS_CPY_AB:
			(*address)++;
			temp = (uint8_t) data -> Y - mem[*address];
			data -> N = (temp & 0b10000000 > 0);
			data -> C = (data -> Y >= mem[*address]);
			data -> Z = (data -> Y > mem[*address]);
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
			temp = (uint8_t) (mem[*address] + data -> X);
			data -> A = (data -> A & mem[getWord(data, &temp, mem)]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			break;
		case INS_AND_IY:
			(*address)++;
			temp = (mem[*address]) & 0b11111111;
			data -> A = (data -> A & mem[(getWord(data, &temp, mem) + data -> Y) & 0b11111111]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			break;
		case INS_EOR_IM:
			(*address)++;
			data -> A = (data -> A ^ mem[*address]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			break;
		case INS_EOR_ZP:
			(*address)++;
			data -> A = (data -> A ^ mem[mem[*address]]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			break;
		case INS_EOR_ZX:
			(*address)++;
			data -> A = (data -> A ^ (uint8_t) (mem[mem[*address] + data -> X]));
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			break;
		case INS_EOR_AB:
			(*address)++;
			data -> A = (data -> A ^ (uint8_t) mem[getWord(data, address, mem)]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			break;
		case INS_EOR_AX:
			(*address)++;
			data -> A = (data -> A ^ (uint8_t) mem[getWord(data, address, mem) + data -> X]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			break;
		case INS_EOR_AY:
			(*address)++;
			data -> A = (data -> A ^ (uint8_t) mem[getWord(data, address, mem) + data -> Y]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			break;
		case INS_EOR_IX:
			(*address)++;
			temp = (uint8_t) (mem[*address] + data -> X);
			data -> A = (data -> A ^ mem[getWord(data, &temp, mem)]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			break;
		case INS_EOR_IY:
			(*address)++;
			temp = (mem[*address]) & 0b11111111;
			data -> A = (data -> A ^ mem[(getWord(data, &temp, mem) + data -> Y) & 0b11111111]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			break;
		case INS_ORA_IM:
			(*address)++;
			data -> A = (data -> A | mem[*address]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			break;
		case INS_ORA_ZP:
			(*address)++;
			data -> A = (data -> A | mem[mem[*address]]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			break;
		case INS_ORA_ZX:
			(*address)++;
			data -> A = (data -> A | (uint8_t) (mem[mem[*address] + data -> X]));
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			break;
		case INS_ORA_AB:
			(*address)++;
			data -> A = (data -> A | (uint8_t) mem[getWord(data, address, mem)]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			break;
		case INS_ORA_AX:
			(*address)++;
			data -> A = (data -> A | (uint8_t) mem[getWord(data, address, mem) + data -> X]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			break;
		case INS_ORA_AY:
			(*address)++;
			data -> A = (data -> A | (uint8_t) mem[getWord(data, address, mem) + data -> Y]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			break;
		case INS_ORA_IX:
			(*address)++;
			temp = (uint8_t) (mem[*address] + data -> X);
			data -> A = (data -> A | mem[getWord(data, &temp, mem)]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			break;
		case INS_ORA_IY:
			(*address)++;
			temp = (mem[*address]) & 0b11111111;
			data -> A = (data -> A | mem[(getWord(data, &temp, mem) + data -> Y) & 0b11111111]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			break;
		case INS_PHA_IP:
			stackPush(data, mem, data -> A, testing_mode);
			break;
		case INS_PLA_IP:
			data -> A = stackPop(data, mem, testing_mode);
			break;
		case INS_PHP_IP:
			stackPush(data, mem, getPS(*data), testing_mode);
			break;
		case INS_PLP_IP:
			setPS(data, stackPop(data, mem, testing_mode));
			break;
		case INS_BVS_RL:
			(*address)++;
			if (data -> V) { 
				if (mem[*address] & 0b10000000) 
				{ 
					*address -= mem[*address] & 0b01111111;
				} 
				else 
				{  
					*address += mem[*address] & 0b01111111;
				} 
			}
			break;
		case INS_BVC_RL:
			(*address)++;
			if (!(data -> V)) { 
				if (mem[*address] & 0b10000000) 
				{ 
					*address -= mem[*address] & 0b01111111;
				} 
				else 
				{  
					*address += mem[*address] & 0b01111111;
				} 
			}
			break;
		case INS_BCS_RL:
			(*address)++;
			if (data -> C) { 
				if (mem[*address] & 0b10000000) 
				{ 
					*address -= mem[*address] & 0b01111111;
				} 
				else 
				{  
					*address += mem[*address] & 0b01111111;
				} 
			}
			break;
		case INS_BEQ_RL:
			(*address)++;
			if (data -> Z) { 
				if (mem[*address] & 0b10000000) 
				{ 
					*address -= mem[*address] & 0b01111111;
				} 
				else 
				{  
					*address += mem[*address] & 0b01111111;
				} 
			}
			break;
		case INS_BMI_RL:
			(*address)++;
			if (data -> N) { 
				if (mem[*address] & 0b10000000) 
				{ 
					*address -= mem[*address] & 0b01111111;
				} 
				else 
				{  
					*address += mem[*address] & 0b01111111;
				} 
			}
			break;
		case INS_BNE_RL:
			(*address)++;
			if (!(data -> Z)) { 
				if (mem[*address] & 0b10000000) 
				{ 
					*address -= mem[*address] & 0b01111111;
				} 
				else 
				{  
					*address += mem[*address] & 0b01111111;
				} 
			}
			break;
		case INS_BPL_RL:
			(*address)++;
			if (!(data -> N)) { 
				if (mem[*address] & 0b10000000) 
				{ 
					*address -= mem[*address] & 0b01111111;
				} 
				else 
				{  
					*address += mem[*address] & 0b01111111;
				} 
			}
			break;
		case INS_BIT_ZP:
			(*address)++;
			temp = data -> A & mem[mem[*address]];
			data -> V = (temp & 0b01000000) > 0;
			data -> N = (temp & 0b10000000) > 0;
			data -> Z = (temp == 0);
			break;
		case INS_BIT_AB:
			(*address)++;
			temp = data -> A & mem[getWord(data, address, mem)];
			data -> V = (temp & 0b01000000) > 0;
			data -> N = (temp & 0b10000000) > 0;
			data -> Z = (temp == 0);
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
			temp = (data -> X + mem[*address]) & 0b11111111;
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
			output = data -> A + mem[(getWord(data, &temp, mem) + data -> Y) & 0b11111111];
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
		case INS_SBC_IM:
			(*address)++;
			output = (data -> A - (!(data -> C) * 256)) - mem[*address];
			data -> C = (output >= 256);
			data -> V = ((data -> A & 0b10000000) != (output & 0b10000000));
			data -> A = output;
			data -> N = (data -> A & 0b10000000 > 1);
			data -> Z = (data -> A == 0);
			if (testing_mode > 1) {
				printf("accumulator: %02x\n", data -> A);
			}
			break;
		case INS_SBC_ZP:
			(*address)++;
			output = (data -> A + (!(data -> C) * 256)) - mem[mem[*address]];
			data -> C = (output >= 256);
			data -> V = ((data -> A & 0b10000000) != (output & 0b10000000));
			data -> A = output;
			data -> N = (data -> A & 0b10000000 > 1);
			data -> Z = (data -> A == 0);
			if (testing_mode > 1) {
				printf("accumulator: %02x\n", data -> A);
			}
			break;
		case INS_SBC_ZX:
			(*address)++;
			output = (data -> A + (!(data -> C) * 256)) - (uint8_t) (mem[mem[*address] + data -> X]);
			data -> C = (output >= 256);
			data -> V = ((data -> A & 0b10000000) != (output & 0b10000000));
			data -> A = output;
			data -> N = (data -> A & 0b10000000 > 1);
			data -> Z = (data -> A == 0);
			if (testing_mode > 1) {
				printf("accumulator: %02x\n", data -> A);
			}
			break;
		case INS_SBC_AB:
			(*address)++;
			output = (data -> A + (!(data -> C) * 256)) - mem[getWord(data, address, mem)];
			data -> C = (output >= 256);
			data -> V = ((data -> A & 0b10000000) != (output & 0b10000000));
			data -> A = output;
			data -> N = (data -> A & 0b10000000 > 1);
			data -> Z = (data -> A == 0);
			if (testing_mode > 1) {
				printf("accumulator: %02x\n", data -> A);
			}
			break;
		case INS_SBC_AX:
			(*address)++;
			tempAdr = (*address) + data -> X;
			output = (data -> A + (!(data -> C) * 256)) - mem[getWord(data, &tempAdr, mem)];
			data -> C = (output >= 256);
			data -> V = ((data -> A & 0b10000000) != (output & 0b10000000));
			data -> A = output;
			data -> N = (data -> A & 0b10000000 > 1);
			data -> Z = (data -> A == 0);
			if (testing_mode > 1) {
				printf("accumulator: %02x\n", data -> A);
			}
			break;
		case INS_SBC_AY:
			(*address)++;
			tempAdr = (*address) + data -> Y;
			output = (data -> A + (!(data -> C) * 256)) - mem[getWord(data, &tempAdr, mem)];
			data -> C = (output >= 256);
			data -> V = ((data -> A & 0b10000000) != (output & 0b10000000));
			data -> A = output;
			data -> N = (data -> A & 0b10000000 > 1);
			data -> Z = (data -> A == 0);
			if (testing_mode > 1) {
				printf("accumulator: %02x\n", data -> A);
			}
			break;
		case INS_SBC_IX:
			(*address)++;
			temp = (data -> X + mem[*address]) & 0b11111111;
			output = (data -> A + (!(data -> C) * 256)) - mem[getWord(data, &temp, mem)];
			data -> C = (output >= 256);
			data -> V = ((data -> A & 0b10000000) != (output & 0b10000000));
			data -> A = output;
			data -> N = (data -> A & 0b10000000 > 1);
			data -> Z = (data -> A == 0);
			if (testing_mode > 1) {
				printf("accumulator: %02x\n", data -> A);
			}
			break;
		case INS_SBC_IY:
			(*address)++;
			temp = mem[*address];
			output = (data -> A + (!(data -> C) * 256)) - mem[(getWord(data, &temp, mem) + data -> Y) & 0b11111111];
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
			data -> Z = (data -> X == 0);
			data -> N = ((data -> X & 0b10000000) > 0);
			break;
		case INS_LDX_ZP:
			(*address)++;
			data -> X = mem[mem[*address]];
			data -> Z = (data -> X == 0);
			data -> N = ((data -> X & 0b10000000) > 0);
			break;
		case INS_LDX_ZY:
			(*address)++;
			data -> X = mem[mem[(*address + data -> Y) & 0b11111111]];
			data -> Z = (data -> X == 0);
			data -> N = ((data -> X & 0b10000000) > 0);
			break;
		case INS_LDX_AB:
			(*address)++;
			data -> X = mem[getWord(data, address, mem)];
			data -> Z = (data -> X == 0);
			data -> N = ((data -> X & 0b10000000) > 0);
			break;
		case INS_LDX_AY:
			(*address)++;
			data -> X = mem[getWord(data, address, mem) + data -> Y];
			data -> Z = (data -> X == 0);
			data -> N = ((data -> X & 0b10000000) > 0);
			break;
		case INS_LDY_IM:
			(*address)++;
			data -> Y = mem[*address];
			data -> Z = (data -> Y == 0);
			data -> N = ((data -> Y & 0b10000000) > 0);
			break;
		case INS_LDY_ZP:
			(*address)++;
			data -> Y = mem[mem[*address]];
			data -> Z = (data -> Y == 0);
			data -> N = ((data -> Y & 0b10000000) > 0);
			break;
		case INS_LDY_ZX:
			(*address)++;
			data -> Y = mem[mem[(*address + data -> X) & 0b11111111]];
			data -> Z = (data -> Y == 0);
			data -> N = ((data -> Y & 0b10000000) > 0);
			break;
		case INS_LDY_AB:
			(*address)++;
			data -> Y = mem[getWord(data, address, mem)];
			data -> Z = (data -> Y == 0);
			data -> N = ((data -> Y & 0b10000000) > 0);
			break;
		case INS_LDY_AX:
			(*address)++;
			data -> Y = mem[getWord(data, address, mem) + data -> X];
			data -> Z = (data -> Y == 0);
			data -> N = ((data -> Y & 0b10000000) > 0);
			break;
		case INS_LDA_IM:
			(*address)++;
			data -> A = mem[*address];
			data -> Z = (data -> A == 0);
			data -> N = ((data -> A & 0b10000000) > 0);
			break;
		case INS_LDA_ZP:
			(*address)++;
			data -> A = mem[mem[*address]];
			data -> Z = (data -> A == 0);
			data -> N = ((data -> A & 0b10000000) > 0);
			break;
		case INS_LDA_ZX:
			(*address)++;
			data -> A = mem[(mem[*address] + data -> X) & 0b11111111];
			data -> Z = (data -> A == 0);
			data -> N = ((data -> A & 0b10000000) > 0);
			break;
		case INS_LDA_AB:
			(*address)++;
			data -> A = mem[getWord(data, address, mem)];
			data -> Z = (data -> A == 0);
			data -> N = ((data -> A & 0b10000000) > 0);
			break;
		case INS_LDA_AX:
			(*address)++;
			data -> A = mem[getWord(data, address, mem) + data -> X];
			data -> Z = (data -> A == 0);
			data -> N = ((data -> A & 0b10000000) > 0);
			break;
		case INS_LDA_AY:
			(*address)++;
			data -> A = mem[getWord(data, address, mem) + data -> Y];
			data -> Z = (data -> A == 0);
			data -> N = ((data -> A & 0b10000000) > 0);
			break;
		case INS_LDA_IX:
			(*address)++;
			data -> A = mem[mem[*address + data -> X]];
			data -> Z = (data -> A == 0);
			data -> N = ((data -> A & 0b10000000) > 0);
			break;
		case INS_LDA_IY:
			(*address)++;
			data -> A = mem[(mem[*address] + data -> Y) & 0b11111111];
			data -> Z = (data -> A == 0);
			data -> N = ((data -> A & 0b10000000) > 0);
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