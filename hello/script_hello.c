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
uint8_t (* const hostapi_get_cnt)(void) = (void *)(HOSTAPI_BASE + 4);
void (* const hostapi_puts)(const char *) = (void *)(HOSTAPI_BASE + 5);
char (* const hostapi_mirror_c)(char) = (void *)(HOSTAPI_BASE + 6);
int8_t (* const hostapi_mirror_b)(int8_t) = (void *)(HOSTAPI_BASE + 7);
uint8_t (* const hostapi_mirror_B)(uint8_t) = (void *)(HOSTAPI_BASE + 8);
int16_t (* const hostapi_mirror_h)(int16_t) = (void *)(HOSTAPI_BASE + 9);
uint16_t (* const hostapi_mirror_H)(uint16_t) = (void *)(HOSTAPI_BASE + 10);
int32_t (* const hostapi_mirror_i)(int32_t) = (void *)(HOSTAPI_BASE + 11);
uint32_t (* const hostapi_mirror_I)(uint32_t) = (void *)(HOSTAPI_BASE + 12);
float (* const hostapi_mirror_f)(float) = (void *)(HOSTAPI_BASE + 13);
int64_t (* const hostapi_mirror_q)(int64_t) = (void *)(HOSTAPI_BASE + 14);
uint64_t (* const hostapi_mirror_Q)(uint64_t) = (void *)(HOSTAPI_BASE + 15);
void * (* const hostapi_mirror_p)(void *) = (void *)(HOSTAPI_BASE + 16);


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

static void panic_if(int expr)
{
    if (expr)
        hostapi_puts("PANIC");
}

void main(void)
{
    while (1)
    {
        uint8_t i = hostapi_get_cnt();

        put_d(i);
        put_s(":");
        hostapi_puts(strings[i % 4]);
//      put_s();
        put_s("\n");
        hostapi_sleep(250);
//      hostapi_regs(1, 1234123433, ~0, 221, 32768, 0, 12);
        hostapi_regs_and_stack(1, 1234123433, ~0, 221, 32768, 0, 666655544, 0xF00F, 126, 125);

        hostapi_sleep(1000);

        char c = 'A';
        panic_if(hostapi_mirror_c(c) != c);
        int8_t b = -128;
        panic_if(hostapi_mirror_b(b) != b);
        uint8_t B = 250;
        panic_if(hostapi_mirror_B(B) != B);
        int16_t h = -32760;
        panic_if(hostapi_mirror_h(h) != h);
        uint16_t H = 65001;
        panic_if(hostapi_mirror_H(H) != H);
        int32_t l = -10UL;
        panic_if(hostapi_mirror_i(l) != l);
        uint32_t I = 0x55AA55AAUL;
        panic_if(hostapi_mirror_I(I) != I);
        float f = 3.1415;
        panic_if(hostapi_mirror_f(f) != f);
        int64_t q = 0x55AA55AA00FFBBEEULL;
        panic_if(hostapi_mirror_q(q) != q);
        uint64_t Q = 0xAA00FFBBEE55AA55ULL;
        panic_if(hostapi_mirror_Q(Q) != Q);
        void *p = &i;
        panic_if(hostapi_mirror_p(p) != p);

        if (KEYB_IO == 'q' || KEYB_IO == 'Q')
            asm("break");
    }
}
