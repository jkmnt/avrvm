#include <stdint.h>

#define HOSTAPI_BASE    0x8000
#define IO_BASE 32
// I/O registers
#define KEYB_IO *((volatile uint8_t *) (IO_BASE + 0))
// Host functions
void (* const hostapi_putc)(char byte) = (void *)(HOSTAPI_BASE + 0);
void (* const hostapi_sleep)(uint32_t ms) = (void *)(HOSTAPI_BASE + 1);

void (* const hostapi_regs)(
            uint16_t a,
            uint32_t b,
            uint32_t c,
            uint8_t d,
            uint16_t e,
            uint8_t f,
            uint8_t g) = (void *)(HOSTAPI_BASE + 2);

void (* const hostapi_regs_and_stack)(
            uint16_t a, // 2
            uint32_t b, // 4
            uint32_t c, // 4
            uint8_t d,  // 2
            uint16_t e, // 2
            uint8_t f,  // 2
            uint32_t g,  // 4
            uint16_t h, uint8_t i, uint8_t j) = (void *)(HOSTAPI_BASE + 3);


static const char *strings[4] = {
    "Hello, sun",
    "Hello, bird",
    "Hello, my lady",
    "Hello, my breakfast. May I buy you again tomorrow ?",
};

static void put_s(const char *str)
{
    while (*str)
        hostapi_putc(*str++);
}

static void put_d(uint8_t byte)
{
    hostapi_putc(byte / 100 + '0');
    byte %= 100;
    hostapi_putc(byte / 10 + '0');
    byte %= 10;
    hostapi_putc(byte + '0');
}

void main(void)
{
    uint8_t i = 0;

    while (1)
    {
        put_d(i++);
        put_s(":");
        put_s(strings[i % 4]);
        put_s("\n");
        hostapi_sleep(250);
//      hostapi_regs(1, 1234123433, ~0, 221, 32768, 0, 12);
        hostapi_regs_and_stack(1, 1234123433, ~0, 221, 32768, 0, 666655544, 0xF00F, 126, 125);

        hostapi_sleep(1000);

        if (KEYB_IO == 'q' || KEYB_IO == 'Q')
            asm("break");
    }
}
