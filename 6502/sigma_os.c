#include "cpu6502.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>

void loadProg(uint8_t* mem);

/*
Custom input output range (Inside the RAM range). Address 5FFF is "wired up" 
to the screen's on/off signal (1 on, 0 off). Address 5FFE is "wired up" 
to the pointer to the byte to display. (Add the value to the starting address
of the IO range.)
*/
const uint32_t IO_RANGE[2] = {0x3FFF00, 0x3FFFFF};

int main() {
	// Set the testing mode: 0 is no debug info, 1 is some (e.g printing the address), 
	// 2 is more (e.g printing addresses jumped to), 3 is most (e.g printing values 
	// pushed to/pulled from the stack), 4 is everything (e.g printing the registers)
	uint8_t testing_mode = 0;
	
	// Make the data struct that contains all of the register info
	struct data data;
	data.clk = 0;
	data.cyclenum = 0;

	// Make the memory
	uint8_t *mem = (uint8_t*) malloc(1024 * 1024 * 16); // 16 megs wow!

	// This is just so the program is out of the way and it's easier to navigate main.
	FILE *fptr;

	fptr = fopen("prog.txt", "r");

	if (fptr == NULL) {
		perror("AHHH ABORT ABORT FAILED TO OPEN FILE!!! AH!!!!");
		return 1;
	}

	loadProgFromFile(data, mem, fptr);

	fclose(fptr);

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
                printf("Addr cleared: %06x\n", data.PC);
            }
		}
        if (testing_mode > 3)
        {
            printf("on: %02x\n", mem[IO_RANGE[1]]);
            printf("char: %02x\n", mem[IO_RANGE[1] - 1]);
        }
        if (testing_mode > 0) {
			printf("\n");
		}
		if (data.cyclenum > 50) {
			break;
		}
	}

	// Print some debug info
	printf("Clock cycles: %d\n", data.cyclenum);
	printf("Final address: %06x\n", (data.PC - 1) & 0xFFFFFF);

	// Output onto the terminal
	printf("TERMINAL OUTPUT:\n");
    for (int i = 0; i < nextFree; i++) {
        printf("%c", string[i]);
    }

	free(mem);

	return data.exit_code;
}

void loadProg(uint8_t *mem) {
 	
	// This is a vector to the reset function.

	mem[0xFFFFFA] = 0x00;
	mem[0xFFFFFB] = 0xF0;
	mem[0xFFFFFC] = 0xFF;
	
	// Note: 0xFFFFFE and 0xFFFFFF are used for the interrupt vector.

	// Print "(keyboard input)\n" then turn it off
	
	mem[0x400000] = MTA_KYB_IP;
	mem[0x400001] = INS_JSR_AB;
	mem[0x400002] = 0x02;
	mem[0x400003] = 0x01;
	mem[0x400004] = 0x40;
	mem[0x400005] = INS_LDX_IM;
	mem[0x400006] = 0x0A;
	mem[0x400007] = INS_JSR_AB;
	mem[0x400008] = 0xF0;
	mem[0x400009] = 0x00;
	mem[0x40000A] = 0x40;
    mem[0x40000B] = MTA_OFF_IP;

	// Print subroutine
	mem[0x4000F0] = INS_TXA_IP;
	mem[0x4000F1] = INS_STX_AB;
	mem[0x4000F2] = IO_RANGE[1] - 1;
	mem[0x4000F3] = (IO_RANGE[1] - 1) >> 8;
	mem[0x4000F4] = (IO_RANGE[1] - 1) >> 16;
    mem[0x4000F5] = INS_LDA_IM;
	mem[0x4000F6] = 0x00;
    mem[0x4000F7] = INS_STA_AB;
	mem[0x4000F8] = IO_RANGE[1];
	mem[0x4000F9] = IO_RANGE[1] >> 8;
	mem[0x4000FA] = IO_RANGE[1] >> 16;
	mem[0x4000FB] = INS_LDA_IM;
	mem[0x4000FC] = 0x01;
	mem[0x4000FD] = INS_STA_AB;
	mem[0x4000FE] = IO_RANGE[1];
	mem[0x4000FF] = IO_RANGE[1] >> 8;
	mem[0x400100] = IO_RANGE[1] >> 16;
	mem[0x400101] = INS_RTS_IP;

	// Print keyboard input subroutine
	mem[0x400102] = INS_LDY_IM;
	mem[0x400103] = 0x00;
	mem[0x400104] = INS_LDX_AY;
	mem[0x400105] = IO_RANGE[0];
	mem[0x400106] = IO_RANGE[0] >> 8;
	mem[0x400107] = IO_RANGE[0] >> 16;
	mem[0x400108] = INS_JSR_AB;
	mem[0x400109] = 0xF0;
	mem[0x40010A] = 0x00;
	mem[0x40010B] = 0x40;
    mem[0x40010C] = INS_INY_IP;
    mem[0x40010D] = INS_LDA_IM;
	mem[0x40010E] = 0x00;
	mem[0x40010F] = INS_ADC_AY;
	mem[0x400110] = IO_RANGE[0];
	mem[0x400111] = IO_RANGE[0] >> 8;
	mem[0x400112] = IO_RANGE[0] >> 16;
	mem[0x400113] = INS_BNE_RL;
    mem[0x400114] = 0b10010000;
	mem[0x400115] = INS_RTS_IP;

	// Custom reset code
	mem[0xFFF000] = INS_LDX_IM;
	mem[0xFFF001] = 0x00;
	mem[0xFFF002] = INS_LDY_IM;
	mem[0xFFF003] = 0x00;
	mem[0xFFF004] = INS_LDA_IM;
	mem[0xFFF005] = 0x00;
	mem[0xFFF006] = INS_CLD_IP;
	mem[0xFFF007] = INS_CLI_IP;
	mem[0xFFF008] = INS_CLC_IP;
	mem[0xFFF009] = INS_CLV_IP;
	mem[0xFFF00A] = INS_JMP_AB;
	mem[0xFFF00B] = 0x00;
	mem[0xFFF00C] = 0x00;
	mem[0xFFF00D] = 0x40;

	return;
}
