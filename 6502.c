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

// Opcodes:

// System control
#define INS_NOP_IP 0x00
#define INS_BRK_IP 0x01

// Processing status
#define INS_CLD_IP 0xD8
#define INS_CLC_IP 0x18
#define INS_CLI_IP 0x58
#define INS_CLV_IP 0xB8

#define INS_SED_IP 0xF8
#define INS_SEC_IP 0x38
#define INS_SEI_IP 0x78

// Jump
#define INS_JMP_AB 0x4C

// Jump to/return from subroutine
#define INS_JSR_AB 0x20
#define INS_RTS_IP 0x60

// Store Y
#define INS_STY_AB 0x84

// Store A
#define INS_STA_AB 0x85

// Store X
#define INS_STX_AB 0x86

// Load Y
#define INS_LDY_IM 0xA0

// Load X
#define INS_LDX_IM 0xA2

// Load A
#define INS_LDA_IM 0xA9
#define INS_LDA_ZP 0xA5
#define INS_LDA_ZX 0xB5
#define INS_LDA_AB 0xAD
#define INS_LDA_AX 0xBD
#define INS_LDA_AY 0xB9
#define INS_LDA_IX 0xA1
#define INS_LDA_IY 0xB1

// Weird instructions (Necessary because of compiler restrictions but not realistic)
#define WRD_WFT_IP 0xFF // Wait for text input
#define WRD_PRT_IM 0x23 // Print immediate text value
#define WRD_PRT_AB 0x22
#define WRD_PRT_AX 0x27
#define WRD_PRT_AY 0x2F
#define WRD_PRT_IX 0x2B
#define WRD_PRT_IY 0x32

// Memory:
const uint16_t MAX_MEM = 0xFFFF;
const uint16_t ZPAGE_RANGE[2] = {0x0000, 0x00FF};
const uint16_t STACK_RANGE[2] = {0x0100, 0x01FF};
const uint16_t RAM_RANGE[2] = {0x0200, 0x53FF};
const uint16_t IO_RANGE[2] = {0x5400, 0x55FF};
const uint16_t ROM_RANGE[2] = {0x5600, 0xFFFF};
const uint16_t SYSMEM_RANGE[2] = {0xFF00, 0xFFFF};

struct data {
	uint8_t C : 1; // Carry
	uint8_t Z : 1; // Zero
	uint8_t I : 1; // Interrupt disable
	uint8_t D : 1; // Decimal mode
	uint8_t B : 1; // Break command
	uint8_t clk : 1; // Clock going
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

uint8_t getInput(uint8_t *mem, uint8_t pin) {
	return mem[IO_RANGE[0] + pin];
}

void waitForTextInput(uint8_t *mem, uint8_t pin, uint16_t bytes) {
	char* string = (char*) malloc(bytes);
	scanf("%s\n", string);
	for (int i = 0; i < bytes; i++) {
		mem[IO_RANGE[i]] = string[i];
	}
	return;
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

void execute(struct data *data, uint8_t *mem, uint16_t *address, uint8_t testing_mode);

uint8_t* initialise_mem(struct data data, uint8_t* mem) {
	for (uint32_t i = RAM_RANGE[0]; i < RAM_RANGE[1]; i++) {
		mem[i] = 0;
	}

	return mem;
}

struct data reset(struct data data) {
	data.clk = 1;
	data.PC = 0xFFFD;
	data.cyclenum = 0;
	data.exit_code = 0;

	return data;
}

void loadProg(uint8_t* mem);

int main() {
	uint8_t testing_mode = 1;
	
	struct data data;
	data.clk = 0;
	data.cyclenum = 0;

	uint8_t mem[MAX_MEM];

	data = reset(data);
	initialise_mem(data, mem);

	loadProg(mem);

	while (data.clk == 1) {
		execute(&data, mem, &(data.PC), testing_mode);
		if (data.exit_code != 0) {
		    break;
		}
	}

        printf("%02x\n", mem[0x3000]);
        printf("%02x\n", mem[0x2999]);
        printf("%02x\n", mem[0x3001]);
        //printf("%02x\n", data.X);
        
	printf("Clock cycles: %d\n", data.cyclenum);

	printf("Final address: %04x\n", data.PC - 1);

	switch (data.exit_code) {
	case 0:
		printf("Code execution successful\n");
		break;
	case 1:
		printf("Err code 0001: Segmentation fault\n");
		break;
	}

	return data.exit_code;
}

void loadProg(uint8_t *mem) {
        // JMP + Reset start address
	mem[0xFFFD] = INS_JMP_AB;
	mem[0xFFFE] = 0x00;
	mem[0xFFFF] = 0xFF;

	// Reset

	mem[0xFF00] = INS_LDX_IM;
	mem[0xFF01] = 0x00;
	mem[0xFF02] = INS_LDY_IM;
	mem[0xFF03] = 0x00;
	mem[0xFF04] = INS_LDA_IM;
	mem[0xFF05] = 0x00;
	mem[0xFF06] = INS_CLD_IP;
	mem[0xFF07] = INS_CLI_IP;
	mem[0xFF08] = INS_CLC_IP;
	mem[0xFF09] = INS_CLV_IP;
	mem[0xFF0A] = INS_JMP_AB;
	mem[0xFF0B] = 0x00;
	mem[0xFF0C] = 0x80;


    // Test program
    /*mem[0x8000] = INS_LDA_IM;
    mem[0x8001] = 0x49;
    mem[0x8002] = INS_BRK_IP;*/

	// Program
	mem[0x8000] = INS_JSR_AB;
	mem[0x8001] = 0x50;
	mem[0x8002] = 0x80;
	mem[0x8003] = INS_LDX_IM;
	mem[0x8004] = 0x53;
	mem[0x8005] = WRD_PRT_IM;
	mem[0x8006] = 0x41;
	mem[0x8007] = WRD_PRT_IM;
	mem[0x8008] = 0x0A;
	mem[0x8009] = INS_BRK_IP;
	
	// Subroutine
	mem[0x8050] = INS_LDA_IM;
	mem[0x8051] = 0x60;
	mem[0x8052] = INS_STA_AB;
	mem[0x8053] = 0x00;
	mem[0x8054] = 0x30;
	mem[0x8055] = INS_RTS_IP;
	
	return;
}

void execute(struct data *data, uint8_t *mem, uint16_t *address, uint8_t testing_mode) {
	uint8_t instruction = mem[*address];
	(*address)++;

	switch (instruction)
	{
	case INS_JMP_AB:
	{
		uint8_t lowByte = mem[*address];
		(*address)++;
		uint8_t highByte = mem[*address];
		(*address) = (lowByte | (highByte << 8));
		data -> cyclenum += 3;
	}
	break;
        
        case WRD_PRT_IM:
        {
            printf("%c", mem[*address]);
            (*address)++;
        }
        break;
        
        case INS_JSR_AB:
        {
                stackPush(data, mem, (*address) - 1 & 0b00000011);
                stackPush(data, mem, ((*address) - 1) >> 8);
                *address = getWord(data, address, mem);
                
                
                data -> cyclenum += 6;
        }
        break;
        
        case INS_RTS_IP:
        {
                uint8_t lowByte = stackPop(data, mem);
                uint8_t highByte = stackPop(data, mem);
                
                (*address) = (lowByte | (highByte << 8)) + 3;
                
                data -> cyclenum += 6;
        }
        break;
        
	case INS_CLV_IP:
	{
		data -> V = 0;
		data -> cyclenum += 2;
	}
	break;
	case INS_CLD_IP:
	{
		data -> D = 0;
		data -> cyclenum += 2;
	}
	break;
	case INS_CLI_IP:
	{
		data -> I = 0;
		data -> cyclenum += 2;
	}
	break;
	case INS_CLC_IP:
	{
		data -> C = 0;
		data -> cyclenum += 2;
	}
	break;

	case INS_SEC_IP:
	{
		data -> C = 1;
		data -> cyclenum += 2;
	}
	break;
	case INS_SEI_IP:
	{
		data -> I = 1;
		data -> cyclenum += 2;
	}
	break;
	case INS_SED_IP:
	{
		data -> D = 1;
		data -> cyclenum += 2;
	}
	break;

    	case INS_STA_AB:
	{
		storeMem(mem, getWord(data, address, mem), data -> A, data);
		data -> cyclenum += 4;
	}
	break;

	case INS_LDA_IM:
	{
		uint8_t val = mem[*address];
		data -> A = val;
		data -> Z = (data -> A == 0);
		data -> N = (data -> A & 0b10000000) > 0;
		data -> cyclenum += 2;
	}
	break;
	case INS_LDA_ZP:
	{
		uint8_t val = mem[mem[*address]];
		data -> A = val;
		data -> Z = (data -> A == 0);
		data -> N = (data -> A & 0b10000000) > 0;
		data -> cyclenum += 3;
	}
	break;
	case INS_LDA_ZX:
	{
		uint8_t val = mem[data -> X];
		setFlagsLDA(data, val);
		data -> cyclenum += 4;
	}
	break;
	case INS_LDA_AB:
	{
		uint16_t valAddr = getWord(data, address, mem);
		uint8_t val = mem[valAddr];
		setFlagsLDA(data, val);
		data -> cyclenum += 4;
	}
	break;
	case INS_LDA_AX:
	{
		uint16_t valAddr = (getWord(data, address, mem) + data -> X);
		uint8_t val = mem[valAddr];
		setFlagsLDA(data, val);
		data -> cyclenum += 4;
	}
	break;
	case INS_LDA_AY:
	{
		uint16_t valAddr = (getWord(data, address, mem) + data -> Y);
		uint8_t val = mem[valAddr];
		setFlagsLDA(data, val);
		data -> cyclenum += 4;
	}
	break;
	case INS_LDA_IX:
	{
		uint16_t valAddr = getWord(data, address, mem);
		uint8_t highByte = mem[valAddr];
		uint8_t lowByte = data -> X;
		valAddr = (lowByte | (highByte << 8));
		uint8_t val = mem[valAddr];
		setFlagsLDA(data, val);
		data -> cyclenum += 6;
	}
	break;
	case INS_LDA_IY:
	{
		uint16_t valAddr = getWord(data, address, mem);
		uint8_t highByte = data -> X;
		uint8_t lowByte = mem[valAddr];
		valAddr = (lowByte | (highByte << 8));
		uint8_t val = mem[valAddr];
		setFlagsLDA(data, val);
		data -> cyclenum += 5;
	}
	break;

	case INS_LDX_IM:
	{
		uint8_t val = mem[*address];
		data -> X = val;
		data -> Z = (data -> X == 0);
		data -> N = (data -> X & 0b10000000) > 0;
		data -> cyclenum += 2;
	}
	break;

	case INS_LDY_IM:
	{
		uint8_t val = mem[*address];
		data -> Y = val;
		data -> Z = (data -> Y == 0);
		data -> N = (data -> Y & 0b10000000) > 0;
		data -> cyclenum += 2;
	}
	break;

	case INS_STX_AB:
	{
		storeMem(mem, data -> X, mem[*address], data);
		data -> cyclenum += 4;
	}
	break;

	case INS_STY_AB:
	{
		storeMem(mem, data -> Y, mem[*address], data);
		data -> cyclenum += 4;
	}
	break;

	case INS_BRK_IP:
	{
		stackPush(data, mem, *address);
		stackPush(data, mem, getPS(*data));
		data -> clk = 0;
		data -> cyclenum += 4;
	}
	break;

	default:
		data -> cyclenum++;
		//printf("Instruction %02x at address %04x not handled\n", instruction, *address - 1);
	}
	if (testing_mode == 1) {
	    printf("addr: %04x\n", *address);
	    printf("inst: %02x\n", instruction);
	    printf("PS: ", getPS(*data));
	}
	
	return;
}
