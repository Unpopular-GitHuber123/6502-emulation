// Opcodes:

// IMPLEMENTED:

// System control
#define INS_BRK_IP 0x00
#define INS_NOP_IP 0x02

// Meta (there isn't any hardware so these are necessary)
#define MTA_OFF_IP 0x03

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

#define INS_ASL_AC 0x0A
#define INS_ASL_ZP 0x06
#define INS_ASL_ZX 0x16
#define INS_ASL_AB 0x0E
#define INS_ASL_AX 0x1E

// Logical
#define INS_AND_IM 0x29
#define INS_AND_ZP 0x25
#define INS_AND_ZX 0x35
#define INS_AND_AB 0x2D
#define INS_AND_AX 0x3D
#define INS_AND_AY 0x39
#define INS_AND_IX 0x21
#define INS_AND_IY 0x31

#define INS_EOR_IM 0x49
#define INS_EOR_ZP 0x45
#define INS_EOR_ZX 0x55
#define INS_EOR_AB 0x4D
#define INS_EOR_AX 0x5D
#define INS_EOR_AY 0x59
#define INS_EOR_IX 0x41
#define INS_EOR_IY 0x51

// Compares
#define INS_CMP_IM 0xC9
#define INS_CMP_ZP 0xC5
#define INS_CMP_ZX 0xD5
#define INS_CMP_AB 0xCD
#define INS_CMP_AX 0xDD
#define INS_CMP_AY 0xD9
#define INS_CMP_IX 0xC1
#define INS_CMP_IY 0xD1

#define INS_CPX_IM 0xE0
#define INS_CPX_ZP 0xE4
#define INS_CPX_AB 0xEC

#define INS_CPY_IM 0xC0
#define INS_CPY_ZP 0xC4
#define INS_CPY_AB 0xCC

// Branches

#define INS_BCS_RL 0xB0
#define INS_BEQ_RL 0xF0
#define INS_BMI_RL 0x30
#define INS_BNE_RL 0xD0
#define INS_BPL_RL 0x10
#define INS_BVC_RL 0x50
#define INS_BVS_RL 0x70

// Bit test

#define INS_BIT_ZP 0x24
#define INS_BIT_AB 0x2C

// Memory

#define INS_DEC_ZP 0xC6
#define INS_DEC_ZX 0xD6
#define INS_DEC_AB 0xCE
#define INS_DEC_AX 0xDE

#define INS_INC_ZP 0xE6
#define INS_INC_ZX 0xF6
#define INS_INC_AB 0xEE
#define INS_INC_AX 0xFE

// Registers

#define INS_DEX_IP 0xCA
#define INS_DEY_IP 0x88

#define INS_INX_IP 0xE8
#define INS_INY_IP 0xC8

// Load Y
#define INS_LDY_IM 0xA0

// Load X
#define INS_LDX_IM 0xA2
#define INS_LDX_ZP 0xA6
#define INS_LDX_ZY 0xB6
#define INS_LDX_AB 0xAE
#define INS_LDX_AY 0xBE

// Load A
#define INS_LDA_IM 0xA9
#define INS_LDA_ZP 0xA5
#define INS_LDA_ZX 0xB5
#define INS_LDA_AB 0xAD
#define INS_LDA_AX 0xBD
#define INS_LDA_AY 0xB9
#define INS_LDA_IX 0xA1
#define INS_LDA_IY 0xB1

// UNIMPLEMENTED

#define INS_STY_AB 0x84
#define INS_STA_AB 0x85
#define INS_STX_AB 0x86

// X stack shii
#define INS_TSX_IP 0xBA
#define INS_TXS_IP 0x9A
