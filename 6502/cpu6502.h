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

#include <stdint.h>
#include "cpu6502.c"

struct data;

uint8_t getPS(struct data data);

void setPS(struct data *data, uint8_t PS);

void stackPush(struct data *data, uint8_t *mem, uint8_t val, uint8_t testing_mode);

uint8_t stackPop(struct data *data, uint8_t *mem, uint8_t testing_mode);

void storeMem(uint8_t *mem, uint16_t address, uint8_t value, struct data *data);

uint16_t getWord(struct data *data, uint16_t *address, uint8_t *mem);

uint8_t* initialise_mem(struct data data, uint8_t* mem);

void reset(struct data *data, uint8_t *mem);

uint16_t execute(struct data *data, uint8_t *mem, uint16_t *address, uint8_t testing_mode, uint8_t *keyboard_addr);