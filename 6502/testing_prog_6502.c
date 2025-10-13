#include "cpu6502.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

void loadProg(uint8_t* mem);

/*
Custom input output range (Inside the RAM range). Address 5FFF is "wired up" 
to the screen's on/off signal (1 on, 0 off). Address 5FFE is "wired up" 
to the pointer to the byte to display. (Add the value to the starting address
of the IO range.)
*/
const uint16_t IO_RANGE[2] = {0x5F00, 0x5FFF};

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
		execute(&data, mem, &data.PC, testing_mode, &mem[IO_RANGE[0]]);
		// Custom screen component
		if (mem[IO_RANGE[1]]) {
			if (alreadyPrinted == 0) {
				string[nextFree] = mem[IO_RANGE[1] - 1];
				nextFree++;
                if (testing_mode > 1) {
                    printf("char set: %c\n", string[nextFree - 1]);
                }
			}
			alreadyPrinted = 1;
		} else {
			alreadyPrinted = 0;
            if (testing_mode > 2) 
            {
                printf("Addr cleared: %04x\n", data.PC);
            }
		}
        if (testing_mode > 3) 
        {
            printf("on: %02x\n", mem[IO_RANGE[1]]);
            printf("char: %c\n", mem[IO_RANGE[1] - 1]);
        }
        if (data.cyclenum > 10) {
            break;
        }
	}

	// Print some debug info
	printf("Clock cycles: %d\n", data.cyclenum);
	printf("Final address: %04x\n", (data.PC - 1) & 0xFFFF);

	// Output onto the terminal
	printf("TERMINAL OUTPUT:\n");
    for (int i = 0; i < nextFree; i++) {
        printf("%c", string[i]);
    }

	return data.exit_code;
}

void loadProg(uint8_t *mem) {
 	
	// This is a vector to the reset function.

	mem[0xFFFC] = 0x00;
	mem[0xFFFD] = 0xFF;
	
	// Note: 0xFFFE and 0xFFFF are used for the interrupt vector.

	// Print "(keyboard input)\n" then turn it off
	
	mem[0x6000] = MTA_KYB_IP;
	mem[0x6001] = INS_JSR_AB;
	mem[0x6002] = 0xFF;
	mem[0x6003] = 0x60;
	mem[0x6004] = INS_LDX_IM;
	mem[0x6005] = 0x0A;
	mem[0x6006] = INS_JSR_AB;
	mem[0x6007] = 0xF0;
	mem[0x6008] = 0x60;
    mem[0x6009] = MTA_OFF_IP;

	// Print subroutine
	mem[0x60F0] = INS_TXA_IP;
	mem[0x60F1] = INS_STX_AB;
	mem[0x60F2] = IO_RANGE[1] - 1;
	mem[0x60F3] = (IO_RANGE[1] - 1) >> 8;
    mem[0x60F4] = INS_LDA_IM;
	mem[0x60F5] = 0x00;
    mem[0x60F6] = INS_STA_AB;
	mem[0x60F7] = IO_RANGE[1];
	mem[0x60F8] = IO_RANGE[1] >> 8;
	mem[0x60F9] = INS_LDA_IM;
	mem[0x60FA] = 0x01;
	mem[0x60FB] = INS_STA_AB;
	mem[0x60FC] = IO_RANGE[1];
	mem[0x60FD] = IO_RANGE[1] >> 8;
	mem[0x60FE] = INS_RTS_IP;

	// Print keyboard input subroutine
	mem[0x60FF] = INS_LDY_IM;
	mem[0x6100] = 0x00;
	mem[0x6101] = INS_LDX_AY;
	mem[0x6102] = IO_RANGE[0];
	mem[0x6103] = IO_RANGE[0] >> 8;
	mem[0x6104] = INS_JSR_AB;
	mem[0x6105] = 0xF0;
	mem[0x6106] = 0x60;
	mem[0x6107] = INS_ADC_AY;
	mem[0x6108] = IO_RANGE[0];
	mem[0x6109] = IO_RANGE[0] >> 8;
    mem[0x610A] = INS_INY_IP;
	mem[0x610B] = INS_BNE_RL;
    mem[0x610C] = 0b10001011;
	mem[0x610D] = INS_RTS_IP;

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
