/*

	Sigma OS is an operating system designed for the MOS 6502 
	with a 24 bit address bus.

	License: MIT (Do whatever with it basically. DISCLAIMER: 
	I'm not a lawyer. Don't take my summaries for legal
	advice.)

*/

#include "cpu6502.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

/*
Custom input output range (Inside the RAM range). Address 1FFFFF is "wired up" 
to the screen's write signal (1 on, 0 off), and the print signal (10 on, 00 
off.), and the clear signal (100 on, 000 off.). The clear signal only clears 
on printing to the screen. Address 1FFFFE is the byte to display. The write 
signal adds the byte to display to the buffer. The print signal prints it to the
terminal.
*/
const uint32_t IO_RANGE[2] = {0x0FFF00, 0x0FFFFF};

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
	
	initialise_mem(data, mem);

	loadProgFromFile(data, mem, fptr);

	fclose(fptr);

	/* 
	Initialise the memory (zero the drive) and data (Setting the clock cycles to 
	0, activating it, etc). Note: It is important to load the program before resetting 
	data, as it will look for a vector at 0xFFFC and 0xFFFD.
	*/
	reset(&data, mem);

	// Execute the program
	int alreadyPrinted = 0;
	int alreadyPrintedToScr = 0;
	int nextFree = 0;
	char string[(IO_RANGE[1] - IO_RANGE[0]) - 2];
	while (data.clk == 1) {
		execute(&data, mem, &data.PC, testing_mode, &mem[IO_RANGE[0]]);
		// Custom screen component
		if ((mem[IO_RANGE[1]] & 0b00000001) > 0) {
			if (alreadyPrinted == 0) {
				string[nextFree] = mem[IO_RANGE[1] - 1];
				nextFree++;
                if (testing_mode > 1) {
                    printf("char set: %02x\n", string[nextFree - 1]);
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
		if ((mem[IO_RANGE[1]] & 0b0000010) > 0) {
			if (alreadyPrintedToScr == 0) {
				string[nextFree] = '\0';
				printf("%s", string);
				if ((mem[IO_RANGE[1]] & 0b00000100) > 0) {
					memset(string, 0, ((IO_RANGE[1] - IO_RANGE[0]) - 2));
					nextFree = 0;
				}
			}
			alreadyPrintedToScr = 1;
		} else {
			alreadyPrintedToScr = 0;
		}
	}

	// Print some debug info
	printf("Clock cycles: %d\n", data.cyclenum);
	printf("Final address: %06x\n", (data.PC - 1) & 0xFFFFFF);

	printf("addr: %02x\n", data.PC);
	return 0;
}