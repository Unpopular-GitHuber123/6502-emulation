#include "CPU.c"

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