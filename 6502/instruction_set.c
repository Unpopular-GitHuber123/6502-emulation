// Opcodes:

// UNIMPLEMENTED:

// System control
#define INS_BRK_IP 0x00

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

// X stack shii
#define INS_TSX_IP 0xBA
#define INS_TXS_IP 0x9A

int cycles[256] = {
    7,
    6,
    0,
    0,
    0
};