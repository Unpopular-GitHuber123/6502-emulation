#include "cpu6502.c"

void loadProg(uint8_t* mem);

/*
Custom input output range (Inside the RAM range). Address 55FF is "wired up" 
to the screen's on/off signal (1 on, 0 off). Address 55FE are "wired up" 
to the pointer to the byte to display. (Add the value to the starting address
of the IO range.)
*/
const uint16_t IO_RANGE[2] = {0x5500, 0x55FF};

int main() {
	// Set the testing mode: 0 is no debug info, 1 is some, 2 is most, 3 is everything plus the registers.
	uint8_t testing_mode = 3;
	
	// Make the data struct that contains all of the register info
	struct data data;
	data.clk = 0;
	data.cyclenum = 0;

	// Make the memory
	uint8_t mem[MAX_MEM];

	/* 
	Initialise the memory (zero the drive) and data 
	(Setting the clock cycles to 0, activating it, etc)
	*/
	data = reset(data);
	initialise_mem(data, mem);

	// This is just so the program is out of the way and it's easier to navigate main
	loadProg(mem);

	// Run the program until the clock is turned off or it gives an error
	int already_printed = 0;
	char *string = (char*) malloc(256);
	int freeStringAdr = 0;
	while (data.clk == 1) {
		execute(&data, mem, &(data.PC), testing_mode);
		if (data.exit_code != 0) {
			break;
		}
		// Emulating a screen component here
		if (IO_RANGE[1] == 0) {
			already_printed = 0;
		}
		if (already_printed == 0 && IO_RANGE[1] == 1) {
			already_printed = 1;
			string[freeStringAdr] = mem[IO_RANGE[1] - 1];
			freeStringAdr++;
			printf("AAAAHHHHHHHHHHHHH");
		}
	}

	// Print some debug info
	printf("Clock cycles: %d\n", data.cyclenum);
	printf("Final address: %04x\n", data.PC - 1);

	printf("%s", string);

	free(string);

	return data.exit_code;
}

void loadProg(uint8_t *mem) {
    /* 
	JMP + Reset start address (this normally wouldn't contain a jump command, 
	and only a vector, but it's easier this way)
	*/
	mem[0xFFFD] = INS_JMP_AB;
	mem[0xFFFE] = 0x00;
	mem[0xFFFF] = 0xFF;

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
	mem[0xFF0C] = 0x56;

	// Program
	mem[0x5600] = INS_LDA_IM;
	mem[0x5601] = 0x50;
	mem[0x5602] = INS_JSR_AB;
	mem[0x5603] = 0x50;
	mem[0x5604] = 0x56;
	mem[0x5605] = INS_BRK_IP;
	
	// Subroutine example (prints a character)
	mem[0x5650] = INS_STA_AB;
	mem[0x5651] = ((IO_RANGE[1] - 1) & 0b00001111);
	mem[0x5652] = ((IO_RANGE[1] - 1) >> 8);
	mem[0x5653] = INS_LDX_IM;
	mem[0x5654] = 0x01;
	mem[0x5655] = INS_STX_AB;
	mem[0x5656] = IO_RANGE[1] & 0b00001111;
	mem[0x5657] = (IO_RANGE[1] >> 8);
	mem[0x5658] = INS_LDX_IM;
	mem[0x5659] = 0x00;
	mem[0x565A] = INS_STX_AB;
	mem[0x565B] = IO_RANGE[1];
	mem[0x565C] = (IO_RANGE[1] >> 8);
	mem[0x565D] = INS_RTS_IP;

	// Note: I'm not good at programming in assembly so this is probably horribly unoptimised
	
	return;
}