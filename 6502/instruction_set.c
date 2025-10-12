// Opcodes:

// IMPLEMENTED:

// System control
#define INS_BRK_IP 0x00
#define INS_NOP_IP 0x02

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
#define INS_JMP_ID 0x6C

// Jump to/return from subroutine
#define INS_JSR_AB 0x20
#define INS_RTS_IP 0x60

// Arithmetic
#define INS_ADC_IM 0x69
#define INS_ADC_ZP 0x65
#define INS_ADC_ZX 0x75
#define INS_ADC_AB 0x6D
#define INS_ADC_AX 0x7D
#define INS_ADC_AY 0x79
#define INS_ADC_IX 0x61
#define INS_ADC_IY 0x71

// Logical
#define INS_AND_IM 0x29
#define INS_AND_ZP 0x25
#define INS_AND_ZX 0x35
#define INS_AND_AB 0x2D
#define INS_AND_AX 0x3D
#define INS_AND_AY 0x39
#define INS_AND_IX 0x21
#define INS_AND_IY 0x31

// Load Y
#define INS_LDY_IM 0xA0

// Load X
#define INS_LDX_IM 0xA2

// Load A
#define INS_LDA_IM 0xA9

// UNIMPLEMENTED

#define INS_LDA_ZP 0xA5
#define INS_LDA_ZX 0xB5
#define INS_LDA_AB 0xAD
#define INS_LDA_AX 0xBD
#define INS_LDA_AY 0xB9
#define INS_LDA_IX 0xA1
#define INS_LDA_IY 0xB1

// Store Y
#define INS_STY_AB 0x84

// Store A
#define INS_STA_AB 0x85

// Store X
#define INS_STX_AB 0x86

// X stack shii
#define INS_TSX_IP 0xBA
#define INS_TXS_IP 0x9A
