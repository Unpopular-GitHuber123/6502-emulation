/*******************************************************

This emulation is similar to the MOS 6502 processor.
It is not a 1:1 emulation, especially with the specs, 
but it is close. It has almost 1:1 accuracy with the
opcodes, with only 2 custom ones.

Also I gave it a 24 bit address bus and 12 bit stack 
pointer because I wanna write an OS in it!!! (I've
never written one before lmao)

It has a maximum addressable memory of 16 MB, including
RAM, programmable ROM, and a stack. These would be external
components in an actual physical circuit.

Credits:
Code: NoWayAmIAGuest/Unpopular-GitHuber123
Helpful video: https://www.youtube.com/watch?v=qJgsuQoy9bc
(Very) helpful website: http://www.6502.org/users/obelisk/index.html

*******************************************************/

#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "instruction_set.c"

// Memory (Change the ranges as you want but be prepared for seg faults and unexpected behaviour):
const uint32_t MAX_MEM = 0xFFFFFF;
const uint32_t ZPAGE_RANGE[2] = {0x000000, 0x0000FF};
const uint32_t STACK_RANGE[2] = {0x000100, 0x0010FF};
const uint32_t RAM_RANGE[2] = {0x001100, 0x3FFFFF};
const uint32_t ROM_RANGE[2] = {0x400000, 0xFFFFFF};
const uint32_t SYSMEM_RANGE[2] = {0xFFF000, 0xFFFFFF};

struct data {
	uint8_t C : 1; // Carry
	uint8_t Z : 1; // Zero
	uint8_t I : 1; // Interrupt disable
	uint8_t D : 1; // Decimal mode
	uint8_t B : 1; // Break command
	uint8_t clk : 1; // Clock going (set to 0 to stop the program)
	uint8_t V : 1; // Overflow flag
	uint8_t N : 1; // Negative flag

	uint32_t PC; // Program counter
	uint16_t SP; // Stack pointer
    
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

	if (data -> SP >= STACK_RANGE[1]) {
		data -> SP = STACK_RANGE[1];
	}

	if (testing_mode > 2) {
		printf("Value pushed to stack: %02x\n", val);
	}

	return;
}

uint8_t stackPop(struct data *data, uint8_t *mem, uint8_t testing_mode) {
	data -> SP++;

	if (data -> SP >= STACK_RANGE[1]) {
		data -> SP = STACK_RANGE[1];
	}

	uint8_t toReturn = mem[data -> SP + STACK_RANGE[0]];

	if (testing_mode > 2) {
		printf("Value returned from stack: %02x\n", toReturn);
	}

	return toReturn;
}

void storeMem(uint8_t *mem, uint32_t address, uint8_t value, struct data *data) {
	mem[address] = value;
	return;
}

uint32_t getAddr(struct data *data, uint32_t *address, uint8_t *mem) {
	uint8_t lowByte = mem[*address];
	(*address)++;
	uint8_t highByte = mem[*address];
	(*address)++;
	uint8_t highHighByte = mem[*address];
	return (lowByte | (highByte << 8) | (highHighByte << 16));
}

uint8_t* initialise_mem(struct data data, uint8_t* mem) {
	for (uint32_t i = RAM_RANGE[0]; i < RAM_RANGE[1]; i++) {
		mem[i] = 0x00;
	}

	return mem;
}

uint32_t hexToDec(char *string, int len) {
	uint32_t val = 0;

	for (int idx = 0; idx < len; idx++) { // yes I know this could be a for loop but idc
		switch (string[idx]) {
			case '1':
				val += pow(16, len - idx);
				break;
			case '2':
				val += pow(16, len - idx) * 2;
				break;
			case '3':
				val += pow(16, len - idx) * 3;
				break;
			case '4':
				val += pow(16, len - idx) * 4;
				break;
			case '5':
				val += pow(16, len - idx) * 5;
				break;
			case '6':
				val += pow(16, len - idx) * 6;
				break;
			case '7':
				val += pow(16, len - idx) * 7;
				break;
			case '8':
				val += pow(16, len - idx) * 8;
				break;
			case '9':
				val += pow(16, len - idx) * 9;
				break;
			case 'a':
				val += pow(16, len - idx) * 10;
				break;
			case 'b':
				val += pow(16, len - idx) * 11;
				break;
			case 'c':
				val += pow(16, len - idx) * 12;
				break;
			case 'd':
				val += pow(16, len - idx) * 13;
				break;
			case 'e':
				val += pow(16, len - idx) * 14;
				break;
			case 'f':
				val += pow(16, len - idx) * 15;
				break;
			default:
				break;
		}
	}
	switch (string[len]) {
		case '1':
			val += 1;
			break;
		case '2':
			val += 2;
			break;
		case '3':
			val += 3;
			break;
		case '4':
			val += 4;
			break;
		case '5':
			val += 5;
			break;
		case '6':
			val += 6;
			break;
		case '7':
			val += 7;
			break;
		case '8':
			val += 8;
			break;
		case '9':
			val += 9;
			break;
		case 'a':
			val += 10;
			break;
		case 'b':
			val += 11;
			break;
		case 'c':
			val += 12;
			break;
		case 'd':
			val += 13;
			break;
		case 'e':
			val += 14;
			break;
		case 'f':
			val += 15;
			break;
		default:
			break;
		}

		// yeah I know there are better ways to do this but I'm lazy and I just want to get this done

	return val;
}

void loadProgFromFile(struct data data, uint8_t* mem, FILE *fp) {
	uint32_t address = 0x000000;
	
	char line[50];
	char c;
	while (fgets(line, 50, fp)) {
		uint8_t lineAdr = 0;
		c = line[lineAdr];
		char tok[50];
		if (c == 'm') {
			while (c != ';' && c != '\n') {
				c = line[lineAdr];
				if (c != ';' && c != '\n') {
					tok[lineAdr] = c;
				}
				lineAdr++;
			}
			address = hexToDec(tok, lineAdr - 2);
		} else {
			while (c != ';' && c != '\n') {
				c = line[lineAdr];
				if (c != ';' && c != '\n') {
					tok[lineAdr] = c;
				}
				lineAdr++;
			}
			mem[address] = hexToDec(tok, lineAdr - 2);
			address++;
		}
	}

	return;
}

void save(uint8_t *mem, FILE *fptr) {
	uint32_t lastAddr = 0x000000;
	uint32_t address = 0x000000;
	while (address < MAX_MEM) {
		if (lastAddr == 0) {
			if (mem[address] == 0) {
				address++;
				continue;
			} else {
				lastAddr = address;
				fprintf(fptr, "m%06x;\n%02x;\n", address, mem[address]);
				address++;
				continue;
			}
		}

		if (mem[address] == 0) {
			if ((address - lastAddr) > 10) {
				address++;
				continue;
			} else {
				uint8_t print = 0;
				for (int i = 0; i < 10; i++) {
					address++;
					if (mem[address] != 0) {
						print = 1;
						break;
					}
				}
				if (print) {
					fprintf(fptr, "%02x;\n", mem[address]);
				}
				continue;
			}
		}

		if ((address - lastAddr) > 10) {
			fprintf(fptr, "m%06x;\n%02x;\n", address, mem[address]);
			lastAddr = address;
			address++;
			continue;
		}
		
		fprintf(fptr, "%02x;\n", mem[address]);
		lastAddr = address;
		address++;
	}

	return;
}

void reset(struct data *data, uint8_t *mem) {
	data -> clk = 1;
	uint32_t temp = 0xFFFFFA;
	data -> PC = getAddr(data, &temp, mem);
	data -> cyclenum = 0;
	data -> exit_code = 0;

	return;
}

void execute(struct data *data, uint8_t *mem, uint32_t *address, uint8_t testing_mode, uint8_t *keyboard_addr) {
	if (testing_mode > 0) {
		printf("Instruction: %02x\n", mem[*address]);
		printf("Address: %06x\n", *address);
	}
	switch (mem[*address]) {
		case MTA_OFF_IP:
			data -> clk = 0;
			data -> cyclenum += 1;
			data -> C = 0;
			break;
		case MTA_SAV_IP:
			FILE *fptr;
			fptr = fopen("prog.txt", "w");
			if (fptr == NULL) {
				perror("AHHH ABORT ABORT FAILED TO OPEN FILE!!! AH!!!!");
				return 1;
			}
			save(mem, fptr);
			fclose(fptr);
			break;
		case MTA_OFS_IP:
			data -> clk = 0;
			data -> cyclenum += 1;
			data -> C = 1;
			break;
		case MTA_KYB_IP:
			scanf("%s", keyboard_addr);
			data -> cyclenum += 10;
			break;
		case INS_BRK_IP:
			uint32_t temp = 0xFFFFFD;
			stackPush(data, mem, *address, testing_mode);
			stackPush(data, mem, (*address >> 8), testing_mode);
			stackPush(data, mem, (*address >> 16), testing_mode);
			stackPush(data, mem, getPS(*data), testing_mode);
			data -> B = 1;
			*address = getAddr(data, &temp, mem);
			data -> cyclenum += 7;
			if (testing_mode > 1) {
				printf("Interrupted to: %06x\n", *address);
			}
			break;
		case INS_STA_ZP:
			(*address)++;
			mem[mem[*address]] = data -> A;
			data -> cyclenum += 3;
			break;
		case INS_STA_ZX:
			(*address)++;
			mem[(mem[*address] + data -> X) & 0b11111111] = data -> A;
			data -> cyclenum += 4;
			break;
		case INS_STA_AB:
			(*address)++;
			mem[getAddr(data, address, mem)] = data -> A;
			data -> cyclenum += 4;
			break;
		case INS_STA_AX:
			(*address)++;
			mem[getAddr(data, address, mem) + data -> X] = data -> A;
			data -> cyclenum += 5;
			break;
		case INS_STA_AY:
			(*address)++;
			mem[getAddr(data, address, mem) + data -> Y] = data -> A;
			data -> cyclenum += 5;
			break;
		case INS_STA_IX:
			(*address)++;
			mem[(mem[(*address) + data -> X]) & 0b11111111] = data -> A;
			data -> cyclenum += 6;
			break;
		case INS_STA_IY:
			(*address)++;
			mem[mem[*address] + data -> Y] = data -> A;
			data -> cyclenum += 6;
			break;
		case INS_RTI_IP:
			setPS(data, stackPop(data, mem, testing_mode));
			temp = stackPop(data, mem, testing_mode);
			temp |= (stackPop(data, mem, testing_mode) << 8);
			temp |= (stackPop(data, mem, testing_mode) << 16);
			data -> PC = temp;
			data -> cyclenum += 6;
			break;
		case INS_STX_ZP:
			(*address)++;
			mem[mem[*address]] = data -> X;
			data -> cyclenum += 3;
			break;
		case INS_STX_ZY:
			(*address)++;
			mem[(mem[*address] + data -> Y) & 0b11111111] = data -> X;
			data -> cyclenum += 4;
			break;
		case INS_STX_AB:
			(*address)++;
			mem[getAddr(data, address, mem)] = data -> X;
			data -> cyclenum += 4;
			break;
		case INS_STY_AB:
			(*address)++;
			mem[getAddr(data, address, mem)] = data -> Y;
			data -> cyclenum += 3;
			break;
		case INS_STY_ZP:
			(*address)++;
			mem[mem[*address]] = data -> Y;
			data -> cyclenum += 4;
			break;
		case INS_STY_ZX:
			(*address)++;
			mem[(mem[*address] + data -> X) & 0b11111111] = data -> Y;
			data -> cyclenum += 4;
			break;
		case INS_TAX_IP:
			data -> X = data -> A;
			data -> Z = (data -> X == 0);
			data -> N = ((data -> X & 0b10000000) > 0);
			data -> cyclenum += 2;
			break;
		case INS_TAY_IP:
			data -> Y = data -> A;
			data -> Z = (data -> Y == 0);
			data -> N = ((data -> Y & 0b10000000) > 0);
			data -> cyclenum += 2;
			break;
		case INS_TYA_IP:
			data -> A = data -> Y;
			data -> Z = (data -> A == 0);
			data -> N = ((data -> A & 0b10000000) > 0);
			data -> cyclenum += 2;
			break;
		case INS_TXA_IP:
			data -> A = data -> X;
			data -> Z = (data -> A == 0);
			data -> N = ((data -> A & 0b10000000) > 0);
			data -> cyclenum += 2;
			break;
		case INS_TSX_IP:
			data -> X = data -> SP;
			data -> Z = (data -> X == 0);
			data -> N = ((data -> X & 0b10000000) > 0);
			data -> cyclenum += 2;
			break;
		case INS_TXS_IP:
			data -> SP = data -> X;
			data -> cyclenum += 2;
			break;
		case INS_DEC_ZP:
			(*address)++;
			mem[mem[*address]]--;
			data -> cyclenum += 5;
			break;
		case INS_DEC_ZX:
			(*address)++;
			mem[(mem[*address] + data -> X) & 0b11111111]--;
			data -> cyclenum += 6;
			break;
		case INS_DEC_AB:
			(*address)++;
			mem[getAddr(data, address, mem)]--;
			data -> cyclenum += 6;
			break;
		case INS_DEC_AX:
			(*address)++;
			mem[getAddr(data, address, mem) + data -> X]--;
			data -> cyclenum += 7;
			break;
		case INS_INC_ZP:
			(*address)++;
			mem[mem[*address]]++;
			data -> cyclenum += 5;
			break;
		case INS_INC_ZX:
			(*address)++;
			mem[(mem[*address] + data -> X) & 0b11111111]++;
			data -> cyclenum += 6;
			break;
		case INS_INC_AB:
			(*address)++;
			mem[getAddr(data, address, mem)]++;
			data -> cyclenum += 6;
			break;
		case INS_INC_AX:
			(*address)++;
			mem[getAddr(data, address, mem) + data -> X]++;
			data -> cyclenum += 7;
			break;
		case INS_DEX_IP:
			data -> X--;
			data -> Z = (data -> X == 0);
			data -> B = (data -> X & 0b10000000 > 1);
			data -> cyclenum += 2;
			break;
		case INS_INX_IP:
			data -> X++;
			data -> Z = (data -> X == 0);
			data -> B = (data -> X & 0b10000000 > 1);
			data -> cyclenum += 2;
			break;
		case INS_DEY_IP:
			data -> Y--;
			data -> Z = (data -> Y == 0);
			data -> B = (data -> Y & 0b10000000 > 1);
			data -> cyclenum += 2;
			break;
		case INS_INY_IP:
			data -> Y++;
			data -> Z = (data -> Y == 0);
			data -> B = (data -> Y & 0b10000000 > 1);
			data -> cyclenum += 2;
			break;
		case INS_ROL_AC:
			temp = (data -> A & 0b10000000);
			data -> A << 1;
			data -> A += data -> C;
			data -> C = temp;
			data -> Z = (data -> A == 0);
			data -> cyclenum += 2;
			break;
		case INS_ROL_ZP:
			(*address)++;
			temp = (mem[mem[*address]] & 0b10000000);
			mem[mem[*address]] << 1;
			mem[mem[*address]] += data -> C;
			data -> C = temp;
			data -> Z = (mem[mem[*address]] & 0b10000000 == 0);
			data -> cyclenum += 5;
			break;
		case INS_ROL_ZX:
			(*address)++;
			temp = (mem[(mem[*address] + data -> X) & 0b11111111] & 0b10000000);
			mem[(mem[*address] + data -> X) & 0b11111111] << 1;
			mem[(mem[*address] + data -> X) & 0b11111111] += data -> C;
			data -> C = temp;
			data -> Z = (mem[(mem[*address] + data -> X) & 0b11111111] == 0);
			data -> cyclenum += 6;
			break;
		case INS_ROL_AB:
			(*address)++;
			uint32_t temp2 = getAddr(data, address, mem);
			temp = (mem[temp2] & 0b10000000);
			mem[temp2] << 1;
			mem[temp2] += data -> C;
			data -> C = temp;
			data -> Z = (mem[temp2] == 0);
			data -> cyclenum += 6;
			break;
		case INS_ROL_AX:
			(*address)++;
			temp2 = getAddr(data, address, mem) + data -> X;
			temp = (mem[temp2] & 0b10000000);
			mem[temp2] << 1;
			mem[temp2] += data -> C;
			data -> C = temp;
			data -> Z = (mem[temp2] == 0);
			data -> cyclenum += 7;
			break;
		case INS_ROR_AC:
			temp = (data -> A & 0b10000000);
			data -> A >> 1;
			data -> A += data -> C;
			data -> C = temp;
			data -> Z = (data -> A == 0);
			data -> cyclenum += 2;
			break;
		case INS_ROR_ZP:
			(*address)++;
			temp = (mem[mem[*address]] & 0b10000000);
			mem[mem[*address]] >> 1;
			mem[mem[*address]] += data -> C;
			data -> C = temp;
			data -> Z = (mem[mem[*address]] & 0b10000000 == 0);
			data -> cyclenum += 5;
			break;
		case INS_ROR_ZX:
			(*address)++;
			temp = (mem[(mem[*address] + data -> X) & 0b11111111] & 0b10000000);
			mem[(mem[*address] + data -> X) & 0b11111111] >> 1;
			mem[(mem[*address] + data -> X) & 0b11111111] += data -> C;
			data -> C = temp;
			data -> Z = (mem[(mem[*address] + data -> X) & 0b11111111] == 0);
			data -> cyclenum += 6;
			break;
		case INS_ROR_AB:
			(*address)++;
			temp2 = getAddr(data, address, mem);
			temp = (mem[temp2] & 0b10000000);
			mem[temp2] >> 1;
			mem[temp2] += data -> C;
			data -> C = temp;
			data -> Z = (mem[temp2] == 0);
			data -> cyclenum += 6;
			break;
		case INS_ROR_AX:
			(*address)++;
			temp2 = getAddr(data, address, mem) + data -> X;
			temp = (mem[temp2] & 0b10000000);
			mem[temp2] >> 1;
			mem[temp2] += data -> C;
			data -> C = temp;
			data -> Z = (mem[temp2] == 0);
			data -> cyclenum += 7;
			break;
		case INS_ASL_AC:
			data -> C = ((data -> A & 0b10000000) > 0);
			data -> A << 1;
			data -> Z = (data -> A == 0);
			data -> N = ((data -> A & 0b10000000) > 0);
			data -> cyclenum += 2;
			break;
		case INS_ASL_ZP:
			uint32_t *temp1 = (uint32_t*) &(mem[mem[*address]]);
			(*address)++;
			data -> C = ((*temp1 & 0b10000000) > 0);
			*temp1 << 1;
			data -> Z = (*temp1 == 0);
			data -> N = ((*temp1 & 0b10000000) > 0);
			data -> cyclenum += 5;
			break;
		case INS_ASL_ZX:
			(*address)++;
			temp1 = (uint32_t*) (uint8_t*) &(mem[mem[*address] + data -> X]);
			data -> C = ((*temp1 & 0b10000000) > 0);
			*temp1 << 1;
			data -> Z = (*temp1 == 0);
			data -> N = ((*temp1 & 0b10000000) > 0);
			data -> cyclenum += 6;
			break;
		case INS_ASL_AB:
			(*address)++;
			temp1 = (uint32_t*) &(mem[getAddr(data, address, mem)]);
			data -> C = ((*temp1 & 0b10000000) > 0);
			*temp1 << 1;
			data -> Z = (*temp1 == 0);
			data -> N = ((*temp1 & 0b10000000) > 0);
			data -> cyclenum += 6;
			break;
		case INS_ASL_AX:
			(*address)++;
			temp1 = (uint32_t*) &(mem[getAddr(data, address, mem) + data -> X]);
			data -> C = ((*temp1 & 0b10000000) > 0);
			*temp1 << 1;
			data -> Z = (*temp1 == 0);
			data -> N = ((*temp1 & 0b10000000) > 0);
			data -> cyclenum += 7;
			break;
		case INS_LSR_AC:
			data -> C = ((*temp1 & 0b00000001) > 0);
			data -> A >> 1;
			data -> Z = (*temp1 == 0);
			data -> N = 0;
			data -> cyclenum += 2;
			break;
		case INS_LSR_ZP:
			temp1 = (uint32_t*) &(mem[mem[*address]]);
			(*address)++;
			data -> C = ((*temp1 & 0b00000001) > 0);
			*temp1 >> 1;
			data -> Z = (*temp1 == 0);
			data -> N = 0;
			data -> cyclenum += 5;
			break;
		case INS_LSR_ZX:
			(*address)++;
			temp1 = (uint32_t*) (uint8_t*) &(mem[mem[*address] + data -> X]);
			data -> C = ((*temp1 & 0b00000001) > 0);
			*temp1 >> 1;
			data -> Z = (*temp1 == 0);
			data -> N = 0;
			data -> cyclenum += 6;
			break;
		case INS_LSR_AB:
			(*address)++;
			temp1 = (uint32_t*) &(mem[getAddr(data, address, mem)]);
			data -> C = ((*temp1 & 0b00000001) > 0);
			*temp1 >> 1;
			data -> Z = (*temp1 == 0);
			data -> N = 0;
			data -> cyclenum += 6;
			break;
		case INS_LSR_AX:
			(*address)++;
			temp1 = (uint32_t*) &(mem[getAddr(data, address, mem) + data -> X]);
			data -> C = ((*temp1 & 0b00000001) > 0);
			*temp1 >> 1;
			data -> Z = (*temp1 == 0);
			data -> N = 0;
			data -> cyclenum += 7;
			break;
		case INS_CMP_IM:
			(*address)++;
			temp = (uint8_t) data -> A - mem[*address];
			data -> N = (temp & 0b10000000 > 0);
			data -> C = (data -> A >= mem[*address]);
			data -> Z = (data -> A > mem[*address]);
			data -> cyclenum += 2;
			break;
		case INS_CMP_ZP:
			(*address)++;
			temp = (uint8_t) data -> A - mem[mem[*address]];
			data -> N = (temp & 0b10000000 > 0);
			data -> C = (data -> A >= mem[*address]);
			data -> Z = (data -> A > mem[*address]);
			data -> cyclenum += 3;
			break;
		case INS_CMP_ZX:
			(*address)++;
			temp = (uint8_t) data -> A - mem[(mem[*address] + data -> X) & 0b11111111];
			data -> N = (temp & 0b10000000 > 0);
			data -> C = (data -> A >= mem[*address]);
			data -> Z = (data -> A > mem[*address]);
			data -> cyclenum += 4;
			break;
		case INS_CMP_AB:
			(*address)++;
			temp = (uint8_t) data -> A - mem[getAddr(data, address, mem)];
			data -> N = (temp & 0b10000000 > 0);
			data -> C = (data -> A >= mem[*address]);
			data -> Z = (data -> A > mem[*address]);
			data -> cyclenum += 4;
			break;
		case INS_CMP_AX:
			(*address)++;
			temp = (uint8_t) data -> A - mem[(getAddr(data, address, mem) + data -> X) & 0b11111111];
			data -> N = (temp & 0b10000000 > 0);
			data -> C = (data -> A >= mem[*address]);
			data -> Z = (data -> A > mem[*address]);
			data -> cyclenum += 5;
			break;
		case INS_CMP_AY:
			(*address)++;
			temp = (uint8_t) data -> A - mem[(getAddr(data, address, mem) + data -> Y) & 0b11111111];
			data -> N = (temp & 0b10000000 > 0);
			data -> C = (data -> A >= mem[*address]);
			data -> Z = (data -> A > mem[*address]);
			data -> cyclenum += 5;
			break;
		case INS_CMP_IX:
			(*address)++;
			uint8_t temp3 = mem[(mem[*address] + data -> X) & 0b11111111];
			temp = (data -> A - mem[temp3]) & 0b11111111;
			data -> N = (temp & 0b10000000 > 0);
			data -> C = (data -> A >= mem[*address]);
			data -> Z = (data -> A > mem[*address]);
			data -> cyclenum += 6;
			break;
		case INS_CMP_IY:
			(*address)++;
			temp3 = mem[mem[*address]];
			temp = (data -> A - ((getAddr(data, (uint32_t*) (&temp3), mem) + data -> Y) & 0b11111111)) & 0b11111111;
			data -> N = (temp & 0b10000000 > 0);
			data -> C = (data -> A >= mem[*address]);
			data -> Z = (data -> A > mem[*address]);
			data -> cyclenum += 6;
			break;
		case INS_CPX_IM:
			(*address)++;
			temp = (uint8_t) data -> X - mem[*address];
			data -> N = (temp & 0b10000000 > 0);
			data -> C = (data -> X >= mem[*address]);
			data -> Z = (data -> X > mem[*address]);
			data -> cyclenum += 2;
			break;
		case INS_CPX_ZP:
			(*address)++;
			temp = (uint8_t) data -> X - mem[mem[*address]];
			data -> N = (temp & 0b10000000 > 0);
			data -> C = (data -> X >= mem[*address]);
			data -> Z = (data -> X > mem[*address]);
			data -> cyclenum += 3;
			break;
		case INS_CPX_AB:
			(*address)++;
			temp = (uint8_t) data -> X - mem[*address];
			data -> N = (temp & 0b10000000 > 0);
			data -> C = (data -> X >= mem[*address]);
			data -> Z = (data -> X > mem[*address]);
			data -> cyclenum += 4;
			break;
		case INS_CPY_IM:
			(*address)++;
			temp = (uint8_t) data -> Y - mem[*address];
			data -> N = (temp & 0b10000000 > 0);
			data -> C = (data -> Y >= mem[*address]);
			data -> Z = (data -> Y > mem[*address]);
			data -> cyclenum += 2;
			break;
		case INS_CPY_ZP:
			(*address)++;
			temp = (uint8_t) data -> Y - mem[mem[*address]];
			data -> N = (temp & 0b10000000 > 0);
			data -> C = (data -> Y >= mem[*address]);
			data -> Z = (data -> Y > mem[*address]);
			data -> cyclenum += 3;
			break;
		case INS_CPY_AB:
			(*address)++;
			temp = (uint8_t) data -> Y - mem[*address];
			data -> N = (temp & 0b10000000 > 0);
			data -> C = (data -> Y >= mem[*address]);
			data -> Z = (data -> Y > mem[*address]);
			data -> cyclenum += 4;
			break;
		case INS_AND_IM:
			(*address)++;
			data -> A = (data -> A & mem[*address]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			data -> cyclenum += 2;
			break;
		case INS_AND_ZP:
			(*address)++;
			data -> A = (data -> A & mem[mem[*address]]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			data -> cyclenum += 3;
			break;
		case INS_AND_ZX:
			(*address)++;
			data -> A = (data -> A & (uint8_t) (mem[mem[*address] + data -> X]));
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			data -> cyclenum += 4;
			break;
		case INS_AND_AB:
			(*address)++;
			data -> A = (data -> A & (uint8_t) mem[getAddr(data, address, mem)]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			data -> cyclenum += 4;
			break;
		case INS_AND_AX:
			(*address)++;
			data -> A = (data -> A & (uint8_t) mem[getAddr(data, address, mem) + data -> X]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			data -> cyclenum += 5;
			break;
		case INS_AND_AY:
			(*address)++;
			data -> A = (data -> A & (uint8_t) mem[getAddr(data, address, mem) + data -> Y]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			data -> cyclenum += 5;
			break;
		case INS_AND_IX:
			(*address)++;
			temp = (uint8_t) (mem[*address] + data -> X);
			data -> A = (data -> A & mem[getAddr(data, &temp, mem)]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			data -> cyclenum += 6;
			break;
		case INS_AND_IY:
			(*address)++;
			temp = (mem[*address]) & 0b11111111;
			data -> A = (data -> A & mem[(getAddr(data, &temp, mem) + data -> Y) & 0b11111111]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			data -> cyclenum += 6;
			break;
		case INS_EOR_IM:
			(*address)++;
			data -> A = (data -> A ^ mem[*address]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			data -> cyclenum += 2;
			break;
		case INS_EOR_ZP:
			(*address)++;
			data -> A = (data -> A ^ mem[mem[*address]]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			data -> cyclenum += 3;
			break;
		case INS_EOR_ZX:
			(*address)++;
			data -> A = (data -> A ^ (uint8_t) (mem[mem[*address] + data -> X]));
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			data -> cyclenum += 4;
			break;
		case INS_EOR_AB:
			(*address)++;
			data -> A = (data -> A ^ (uint8_t) mem[getAddr(data, address, mem)]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			data -> cyclenum += 4;
			break;
		case INS_EOR_AX:
			(*address)++;
			data -> A = (data -> A ^ (uint8_t) mem[getAddr(data, address, mem) + data -> X]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			data -> cyclenum += 5;
			break;
		case INS_EOR_AY:
			(*address)++;
			data -> A = (data -> A ^ (uint8_t) mem[getAddr(data, address, mem) + data -> Y]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			data -> cyclenum += 5;
			break;
		case INS_EOR_IX:
			(*address)++;
			temp = (uint8_t) (mem[*address] + data -> X);
			data -> A = (data -> A ^ mem[getAddr(data, &temp, mem)]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			data -> cyclenum += 6;
			break;
		case INS_EOR_IY:
			(*address)++;
			temp = (mem[*address]) & 0b11111111;
			data -> A = (data -> A ^ mem[(getAddr(data, &temp, mem) + data -> Y) & 0b11111111]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			data -> cyclenum += 6;
			break;
		case INS_ORA_IM:
			(*address)++;
			data -> A = (data -> A | mem[*address]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			data -> cyclenum += 2;
			break;
		case INS_ORA_ZP:
			(*address)++;
			data -> A = (data -> A | mem[mem[*address]]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			data -> cyclenum += 3;
			break;
		case INS_ORA_ZX:
			(*address)++;
			data -> A = (data -> A | (uint8_t) (mem[mem[*address] + data -> X]));
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			data -> cyclenum += 4;
			break;
		case INS_ORA_AB:
			(*address)++;
			data -> A = (data -> A | (uint8_t) mem[getAddr(data, address, mem)]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			data -> cyclenum += 4;
			break;
		case INS_ORA_AX:
			(*address)++;
			data -> A = (data -> A | (uint8_t) mem[getAddr(data, address, mem) + data -> X]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			data -> cyclenum += 5;
			break;
		case INS_ORA_AY:
			(*address)++;
			data -> A = (data -> A | (uint8_t) mem[getAddr(data, address, mem) + data -> Y]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			data -> cyclenum += 5;
			break;
		case INS_ORA_IX:
			(*address)++;
			temp = (uint8_t) (mem[*address] + data -> X);
			data -> A = (data -> A | mem[getAddr(data, &temp, mem)]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			data -> cyclenum += 6;
			break;
		case INS_ORA_IY:
			(*address)++;
			temp = (mem[*address]) & 0b11111111;
			data -> A = (data -> A | mem[(getAddr(data, &temp, mem) + data -> Y) & 0b11111111]);
			data -> Z = (data -> A == 0);
			data -> N = (data -> A & 0b10000000 > 0);
			data -> cyclenum += 6;
			break;
		case INS_PHA_IP:
			stackPush(data, mem, data -> A, testing_mode);
			data -> cyclenum += 3;
			break;
		case INS_PLA_IP:
			data -> A = stackPop(data, mem, testing_mode);
			data -> cyclenum += 4;
			break;
		case INS_PHP_IP:
			stackPush(data, mem, getPS(*data), testing_mode);
			data -> cyclenum += 3;
			break;
		case INS_PLP_IP:
			setPS(data, stackPop(data, mem, testing_mode));
			data -> cyclenum += 4;
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
				if (testing_mode > 1) {
						printf("Branched to: %06x\n", *address);
						data -> cyclenum += 1;
				}
			} else {
				if (testing_mode > 1) {
					printf("Failed to branch.\n", *address);
				}
			}
			data -> cyclenum += 4;
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
				if (testing_mode > 1) {
						printf("Branched to: %06x\n", *address);
						data -> cyclenum += 1;
				}
			} else {
				if (testing_mode > 1) {
					printf("Failed to branch.\n", *address);
				}
			}
			data -> cyclenum += 4;
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
				if (testing_mode > 1) {
						printf("Branched to: %06x\n", *address);
						data -> cyclenum += 1;
				}
			} else {
				if (testing_mode > 1) {
					printf("Failed to branch.\n", *address);
				}
			}
			data -> cyclenum += 4;
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
				if (testing_mode > 1) {
						printf("Branched to: %06x\n", *address);
						data -> cyclenum += 1;
				}
			} else {
				if (testing_mode > 1) {
					printf("Failed to branch.\n", *address);
				}
			}
			data -> cyclenum += 4;
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
				if (testing_mode > 1) {
					printf("Branched to: %06x\n", *address);
					data -> cyclenum += 1;
				}
			} else {
				if (testing_mode > 1) {
					printf("Failed to branch.\n", *address);
				}
			}
			data -> cyclenum += 4;
			break;
		case INS_BNE_RL:
			(*address)++;
			if (!(data -> Z)) { 
				if (mem[*address] & 0b10000000) 
				{ 
					*address -= (mem[*address] & 0b01111111) + 1;
				} 
				else 
				{  
					*address += (mem[*address] & 0b01111111) - 1;
				}
				if (testing_mode > 1) {
					printf("Branched to: %06x\n", *address);
					data -> cyclenum += 1;
				}
			} else {
				if (testing_mode > 1) {
					printf("Failed to branch.\n", *address);
				}
			}
			data -> cyclenum += 4;
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
				if (testing_mode > 1) {
					printf("Branched to: %06x\n", *address);
					data -> cyclenum += 1;
				}
			} else {
				if (testing_mode > 1) {
					printf("Failed to branch.\n", *address);
				}
			}
			data -> cyclenum += 4;
			break;
		case INS_BIT_ZP:
			(*address)++;
			temp = data -> A & mem[mem[*address]];
			data -> V = (temp & 0b01000000) > 0;
			data -> N = (temp & 0b10000000) > 0;
			data -> Z = (temp == 0);
			data -> cyclenum += 3;
			break;
		case INS_BIT_AB:
			(*address)++;
			temp = data -> A & mem[getAddr(data, address, mem)];
			data -> V = (temp & 0b01000000) > 0;
			data -> N = (temp & 0b10000000) > 0;
			data -> Z = (temp == 0);
			data -> cyclenum += 4;
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
			data -> cyclenum += 2;
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
			data -> cyclenum += 3;
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
			data -> cyclenum += 4;
			break;
		case INS_ADC_AB:
			(*address)++;
			output = data -> A + mem[getAddr(data, address, mem)];
			if (data -> C == 1) { output += 256; }
			data -> C = (output >= 256);
			data -> V = ((data -> A & 0b10000000) != (output & 0b10000000));
			data -> A = output;
			data -> N = (data -> A & 0b10000000 > 1);
			data -> Z = (data -> A == 0);
			if (testing_mode > 1) {
				printf("accumulator: %02x\n", data -> A);
			}
			data -> cyclenum += 4;
			break;
		case INS_ADC_AX:
			(*address)++;
			output = data -> A + mem[getAddr(data, address, mem) + data -> X];
			if (data -> C == 1) { output += 256; }
			data -> C = (output >= 256);
			data -> V = ((data -> A & 0b10000000) != (output & 0b10000000));
			data -> A = output;
			data -> N = (data -> A & 0b10000000 > 1);
			data -> Z = (data -> A == 0);
			if (testing_mode > 1) {
				printf("accumulator: %02x\n", data -> A);
			}
			data -> cyclenum += 5;
			break;
		case INS_ADC_AY:
			(*address)++;
			output = data -> A + mem[getAddr(data, address, mem)] + data -> Y;
			if (data -> C == 1) { output += 256; }
			data -> C = (output >= 256);
			data -> V = ((data -> A & 0b10000000) != (output & 0b10000000));
			data -> A = output;
			data -> N = (data -> A & 0b10000000 > 1);
			data -> Z = (data -> A == 0);
			if (testing_mode > 1) {
				printf("accumulator: %02x\n", data -> A);
			}
			data -> cyclenum += 5;
			break;
		case INS_ADC_IX:
			(*address)++;
			temp = (data -> X + mem[*address]) & 0b11111111;
			output = data -> A + mem[getAddr(data, &temp, mem)];
			if (data -> C == 1) { output += 256; }
			data -> C = (output >= 256);
			data -> V = ((data -> A & 0b10000000) != (output & 0b10000000));
			data -> A = output;
			data -> N = (data -> A & 0b10000000 > 1);
			data -> Z = (data -> A == 0);
			if (testing_mode > 1) {
				printf("accumulator: %02x\n", data -> A);
			}
			data -> cyclenum += 6;
			break;
		case INS_ADC_IY:
			(*address)++;
			temp = mem[*address];
			output = data -> A + mem[(getAddr(data, &temp, mem) + data -> Y) & 0b11111111];
			if (data -> C == 1) { output += 256; }
			data -> C = (output >= 256);
			data -> V = ((data -> A & 0b10000000) != (output & 0b10000000));
			data -> A = output;
			data -> N = (data -> A & 0b10000000 > 1);
			data -> Z = (data -> A == 0);
			if (testing_mode > 1) {
				printf("accumulator: %02x\n", data -> A);
			}
			data -> cyclenum += 6;
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
			data -> cyclenum += 2;
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
			data -> cyclenum += 3;
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
			data -> cyclenum += 4;
			break;
		case INS_SBC_AB:
			(*address)++;
			output = (data -> A + (!(data -> C) * 256)) - mem[getAddr(data, address, mem)];
			data -> C = (output >= 256);
			data -> V = ((data -> A & 0b10000000) != (output & 0b10000000));
			data -> A = output;
			data -> N = (data -> A & 0b10000000 > 1);
			data -> Z = (data -> A == 0);
			if (testing_mode > 1) {
				printf("accumulator: %02x\n", data -> A);
			}
			data -> cyclenum += 4;
			break;
		case INS_SBC_AX:
			(*address)++;
			output = (data -> A + (!(data -> C) * 256)) - mem[getAddr(data, address, mem)+ data -> X];
			data -> C = (output >= 256);
			data -> V = ((data -> A & 0b10000000) != (output & 0b10000000));
			data -> A = output;
			data -> N = (data -> A & 0b10000000 > 1);
			data -> Z = (data -> A == 0);
			if (testing_mode > 1) {
				printf("accumulator: %02x\n", data -> A);
			}
			data -> cyclenum += 5;
			break;
		case INS_SBC_AY:
			(*address)++;
			output = (data -> A + (!(data -> C) * 256)) - mem[getAddr(data, address, mem) + data -> Y];
			data -> C = (output >= 256);
			data -> V = ((data -> A & 0b10000000) != (output & 0b10000000));
			data -> A = output;
			data -> N = (data -> A & 0b10000000 > 1);
			data -> Z = (data -> A == 0);
			if (testing_mode > 1) {
				printf("accumulator: %02x\n", data -> A);
			}
			data -> cyclenum += 5;
			break;
		case INS_SBC_IX:
			(*address)++;
			temp = (data -> X + mem[*address]) & 0b11111111;
			output = (data -> A + (!(data -> C) * 256)) - mem[getAddr(data, &temp, mem)];
			data -> C = (output >= 256);
			data -> V = ((data -> A & 0b10000000) != (output & 0b10000000));
			data -> A = output;
			data -> N = (data -> A & 0b10000000 > 1);
			data -> Z = (data -> A == 0);
			if (testing_mode > 1) {
				printf("accumulator: %02x\n", data -> A);
			}
			data -> cyclenum += 6;
			break;
		case INS_SBC_IY:
			(*address)++;
			temp = mem[*address];
			output = (data -> A + (!(data -> C) * 256)) - mem[(getAddr(data, &temp, mem) + data -> Y) & 0b11111111];
			data -> C = (output >= 256);
			data -> V = ((data -> A & 0b10000000) != (output & 0b10000000));
			data -> A = output;
			data -> N = (data -> A & 0b10000000 > 1);
			data -> Z = (data -> A == 0);
			if (testing_mode > 1) {
				printf("accumulator: %02x\n", data -> A);
			}
			data -> cyclenum += 6;
			break;
		case INS_JMP_AB:
			(*address)++;
			*address = getAddr(data, address, mem) - 1;
			if (testing_mode > 1) {
				printf("Address jumped to: %06x\n", *address + 1);
			}
			data -> cyclenum += 3;
			break;
		case INS_JMP_ID:
			(*address)++;
			uint32_t addr = getAddr(data, address, mem);
			(*address) = getAddr(data, &addr, mem) - 1;
			if (testing_mode > 1) {
				printf("Address jumped to: %06x\n", *address + 1);
			}
			data -> cyclenum += 5;
			break;
		case INS_JSR_AB:
			(*address)++;
			stackPush(data, mem, (*address) - 1, testing_mode);
			stackPush(data, mem, ((*address) - 1) >> 8, testing_mode);
			stackPush(data, mem, ((*address) - 1) >> 16, testing_mode);
			*address = getAddr(data, address, mem) - 1;
			if (testing_mode > 1) {
				printf("Address jumped to: %06x\n", *address + 1);
			}
			data -> cyclenum += 6;
			break;
		case INS_RTS_IP:
			uint8_t highHighByte = stackPop(data, mem, testing_mode);
			uint8_t highByte = stackPop(data, mem, testing_mode);
			uint8_t lowByte = stackPop(data, mem, testing_mode);
			(*address) = (lowByte | ((highByte << 8) | (highHighByte << 16))) + 3;
			if (testing_mode > 3) {
				printf("Low address byte: %02x\n", lowByte);
				printf("High address byte: %02x\n", highByte);
			}
			if (testing_mode > 1) {
				printf("Address returned to: %06x\n", *address + 1);
			}
			data -> cyclenum += 6;
			break;
		case INS_LDX_IM:
			(*address)++;
			data -> X = mem[*address];
			data -> Z = (data -> X == 0);
			data -> N = ((data -> X & 0b10000000) > 0);
			data -> cyclenum += 2;
			break;
		case INS_LDX_ZP:
			(*address)++;
			data -> X = mem[mem[*address]];
			data -> Z = (data -> X == 0);
			data -> N = ((data -> X & 0b10000000) > 0);
			data -> cyclenum += 3;
			break;
		case INS_LDX_ZY:
			(*address)++;
			data -> X = mem[mem[(*address + data -> Y) & 0b11111111]];
			data -> Z = (data -> X == 0);
			data -> cyclenum += 4;
			data -> N = ((data -> X & 0b10000000) > 0);
			break;
		case INS_LDX_AB:
			(*address)++;
			data -> X = mem[getAddr(data, address, mem)];
			data -> Z = (data -> X == 0);
			data -> N = ((data -> X & 0b10000000) > 0);
			data -> cyclenum += 4;
			break;
		case INS_LDX_AY:
			(*address)++;
			data -> X = mem[getAddr(data, address, mem) + data -> Y];
			data -> Z = (data -> X == 0);
			data -> N = ((data -> X & 0b10000000) > 0);
			data -> cyclenum += 5;
			break;
		case INS_LDY_IM:
			(*address)++;
			data -> Y = mem[*address];
			data -> Z = (data -> Y == 0);
			data -> N = ((data -> Y & 0b10000000) > 0);
			data -> cyclenum += 2;
			break;
		case INS_LDY_ZP:
			(*address)++;
			data -> Y = mem[mem[*address]];
			data -> Z = (data -> Y == 0);
			data -> N = ((data -> Y & 0b10000000) > 0);
			data -> cyclenum += 3;
			break;
		case INS_LDY_ZX:
			(*address)++;
			data -> Y = mem[mem[(*address + data -> X) & 0b11111111]];
			data -> Z = (data -> Y == 0);
			data -> N = ((data -> Y & 0b10000000) > 0);
			data -> cyclenum += 4;
			break;
		case INS_LDY_AB:
			(*address)++;
			data -> Y = mem[getAddr(data, address, mem)];
			data -> Z = (data -> Y == 0);
			data -> N = ((data -> Y & 0b10000000) > 0);
			data -> cyclenum += 4;
			break;
		case INS_LDY_AX:
			(*address)++;
			data -> Y = mem[getAddr(data, address, mem) + data -> X];
			data -> Z = (data -> Y == 0);
			data -> N = ((data -> Y & 0b10000000) > 0);
			data -> cyclenum += 5;
			break;
		case INS_LDA_IM:
			(*address)++;
			data -> A = mem[*address];
			data -> Z = (data -> A == 0);
			data -> N = ((data -> A & 0b10000000) > 0);
			data -> cyclenum += 2;
			break;
		case INS_LDA_ZP:
			(*address)++;
			data -> A = mem[mem[*address]];
			data -> Z = (data -> A == 0);
			data -> N = ((data -> A & 0b10000000) > 0);
			data -> cyclenum += 3;
			break;
		case INS_LDA_ZX:
			(*address)++;
			data -> A = mem[(mem[*address] + data -> X) & 0b11111111];
			data -> Z = (data -> A == 0);
			data -> N = ((data -> A & 0b10000000) > 0);
			data -> cyclenum += 4;
			break;
		case INS_LDA_AB:
			(*address)++;
			data -> A = mem[getAddr(data, address, mem)];
			data -> Z = (data -> A == 0);
			data -> N = ((data -> A & 0b10000000) > 0);
			data -> cyclenum += 4;
			break;
		case INS_LDA_AX:
			(*address)++;
			data -> A = mem[getAddr(data, address, mem) + data -> X];
			data -> Z = (data -> A == 0);
			data -> N = ((data -> A & 0b10000000) > 0);
			data -> cyclenum += 5;
			break;
		case INS_LDA_AY:
			(*address)++;
			data -> A = mem[getAddr(data, address, mem) + data -> Y];
			data -> Z = (data -> A == 0);
			data -> N = ((data -> A & 0b10000000) > 0);
			data -> cyclenum += 5;
			break;
		case INS_LDA_IX:
			(*address)++;
			data -> A = mem[mem[*address + data -> X]];
			data -> Z = (data -> A == 0);
			data -> N = ((data -> A & 0b10000000) > 0);
			data -> cyclenum += 6;
			break;
		case INS_LDA_IY:
			(*address)++;
			data -> A = mem[(mem[*address] + data -> Y) & 0b11111111];
			data -> Z = (data -> A == 0);
			data -> N = ((data -> A & 0b10000000) > 0);
			data -> cyclenum += 6;
			break;
		case INS_CLD_IP:
			data -> D = 0;
			data -> cyclenum += 2;
			break;
		case INS_SED_IP:
			data -> D = 1;
			data -> cyclenum += 2;
			break;
		case INS_CLC_IP:
			data -> C = 0;
			data -> cyclenum += 2;
			break;
		case INS_SEC_IP:
			data -> C = 1;
			data -> cyclenum += 2;
			break;
		case INS_CLI_IP:
			data -> I = 0;
			data -> cyclenum += 2;
			break;
		case INS_SEI_IP:
			data -> I = 1;
			data -> cyclenum += 2;
			break;
		case INS_CLV_IP:
			data -> V = 0;
			data -> cyclenum += 2;
			break;
		case INS_NOP_IP:
			data -> cyclenum += 2;
			break;
		default:
			printf("Unrecognised instruction %02x at address: %06x\n", mem[*address], *address);
	}
	if (testing_mode > 3) {
		printf("C: %d Z: %d I: %d D: %d B: %d clk: %d V: %d N: %d\n", data -> C, data -> Z, data -> I, data -> D, data -> B, data -> clk, data -> V, data -> N);
		printf("PC: %06x\n", data -> PC);
		printf("A: %02x\n", data -> A);
		printf("X: %02x\n", data -> X);
		printf("Y: %02x\n", data -> Y);
		printf("SP: %02x\n", data -> SP);
	}
	(*address)++;
	return;
}