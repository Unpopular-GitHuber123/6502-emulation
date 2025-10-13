#include "cpu6502.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

void loadProg(uint8_t* mem);

/*
Custom input output range (Inside the RAM range). Address 7FFF is "wired up" 
to the screen's on/off signal (1 on, 0 off). Address 7FFE is "wired up" 
to the pointer to the byte to display. (Add the value to the starting address
of the IO range.)
*/
const uint16_t IO_RANGE[2] = {0x7000, 0x7FFF};

int main() {
	// Set the testing mode: 0 is no debug info, 1 is some (e.g printing the address), 
	// 2 is more (e.g printing addresses jumped to), 3 is most (e.g printing values 
	// pushed to/pulled from the stack), 4 is everything (e.g printing the registers)
	uint8_t testing_mode = 4;
	
	// Make the data struct that contains all of the register info
	struct data data;
	data.clk = 0;
	data.cyclenum = 0;

	// Make the memory
	uint8_t mem[MAX_MEM];

	// This is just so the program is out of the way and it's easier to navigate main.
	loadProg(mem);

	/* 
	Initialise the memory (zero the drive) and data (Setting the clock cycles to 
	0, activating it, etc). Note: It is important to load the program before resetting 
	data, as it will look for a vector at 0xFFFC and 0xFFFD.
	*/
	initialise_mem(data, mem);
	reset(&data, mem);

	// Execute the program
	int alreadyPrinted = 0;
	int nextFree = 0;
	char string[IO_RANGE[1] - IO_RANGE[0]];
	while (data.clk == 1) {
		execute(&data, mem, &data.PC, testing_mode);
		// Custom screen component
		if (mem[IO_RANGE[1]]) {
			if (alreadyPrinted == 0) {
				string[nextFree] = mem[IO_RANGE[1] - 1];
				nextFree++;
			}
			alreadyPrinted = 1;
		} else {
			alreadyPrinted = 0;
		}
	}

	// Print some debug info
	printf("Clock cycles: %d\n", data.cyclenum);
	printf("Final address: %04x\n", (data.PC - 1) & 0xFFFF);

	// Output onto the terminal
	printf("TERMINAL OUTPUT:\n%s", string);

	return data.exit_code;
}

void loadProg(uint8_t *mem) {
 	
	// This is a vector to the reset function.

	mem[0xFFFC] = 0x00;
	mem[0xFFFD] = 0xFF;
	
	// Note: 0xFFFE and 0xFFFF are used for the interrupt vector.

	// Print 67\n then turn it off
	
	mem[0x6000] = INS_LDX_IM;
	mem[0x6001] = 0x36;
	mem[0x6002] = INS_JSR_AB;
	mem[0x6003] = 0xF0;
	mem[0x6004] = 0x80;
	mem[0x6005] = INS_LDX_IM;
	mem[0x6006] = 0x37;
	mem[0x6007] = INS_JSR_AB;
	mem[0x6008] = 0xF0;
	mem[0x6009] = 0x80;
	mem[0x600A] = INS_LDX_IM;
	mem[0x600B] = 0x0A;
	mem[0x600C] = INS_JSR_AB;
	mem[0x600D] = 0xF0;
	mem[0x600E] = 0x80;
	mem[0x600F] = MTA_OFF_IP;

	// Print subroutine
	mem[0x60F0] = INS_TXA_IP;
	mem[0x60F1] = INS_STA_AB;
	mem[0x60F2] = 0xFE;
	mem[0x60F3] = 0x7F;
	mem[0x60F4] = INS_LDA_IM;
	mem[0x60F5] = 0x01;
	mem[0x60F6] = INS_STA_AB;
	mem[0x60F7] = 0xFF;
	mem[0x60F8] = 0x7F;
	mem[0x60F9] = INS_LDA_IM;
	mem[0x60FA] = 0x00;
	mem[0x60FB] = INS_STA_AB;
	mem[0x60FC] = 0xFF;
	mem[0x60FD] = 0x7F;
	mem[0x60FE] = INS_RTS_IP;

	// Custom reset code
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
	mem[0xFF0C] = 0x60;



	return;
}
