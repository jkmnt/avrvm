#include <stdint.h>

#define HOSTAPI_BASE    0x8000
#define IO_BASE 32
// I/O registers
#define KEYB_IO *((volatile uint8_t *) (IO_BASE + 0))
// Host functions
void (* const hostapi_putc)(char byte) = (void *)(HOSTAPI_BASE + 0);
void (* const hostapi_sleep)(uint32_t ms) = (void *)(HOSTAPI_BASE + 1);
uint8_t (* const hostapi_get_next_count)(void) = (void *)(HOSTAPI_BASE + 2);

static const char * const strings[4] = {
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
    while (1)
    {
        uint8_t i = hostapi_get_next_count();

        put_d(i);
        put_s(":");
        put_s(strings[i % 4]);
        put_s("\n");
        hostapi_sleep(250);

        if (KEYB_IO == 'q' || KEYB_IO == 'Q')
            asm("break");
    }
}
