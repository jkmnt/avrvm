#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "avrvm.h"

#include <windows.h>    // for Sleep
#include <conio.h>      // for _kbhit and _getch

static FILE *src;

static avrvm_ctx_t vm;
static uint8_t sram[1024 + 160];
static uint8_t io[61];

static uint16_t flash_reader(uint16_t word_idx)
{
    // wrap at 4k words to simulate behavior of atmega88 with 8k flash
    word_idx &= 0x0FFF;
    if (fseek(src, word_idx * 2, SEEK_SET) != 0)
    {
        printf("failed to seek");
        exit(-1);
    }

    uint16_t opcode;
    if (fread ( &opcode, 2, 1, src) != 1)
    {
        printf("failed to read");
        exit(-1);
    }

    return opcode;
}

static uint8_t sram_reader(uint16_t addr)
{
    return sram[addr];
}

static void sram_writer(uint16_t addr, uint8_t byte)
{
    sram[addr] = byte;
}

static uint8_t io_reader(uint8_t addr)
{
    return io[addr];
}

static void io_writer(uint8_t addr, uint8_t byte)
{
    io[addr] = byte;
}

static void exec_ext_call(int idx)
{
    switch (idx)
    {
    case 0:
        printf("%c", vm.regs[24]); // single-byte argument, passed by gcc in r24
        break;

    case 1:
        {
            uint32_t interval = (vm.regs[25] << 24) | (vm.regs[24] << 16) | (vm.regs[23] << 8) | (vm.regs[22]) ;
            Sleep(interval);
        }
        break;
    }
}

static void run_hello(const char *fname)
{
    avrvm_iface_t iface =
    {
        .flash_r = flash_reader,
        .sram_r = sram_reader,
        .sram_w = sram_writer,
        .io_r = io_reader,
        .io_w = io_writer,
    };

    avrvm_init(&vm, &iface, sizeof(sram));
    src = fopen(fname, "rb");
    if (!src)
    {
        printf("failed to open");
        return;
    }

    while (1)
    {
        if (_kbhit())
        {
            io[0] = _getch();   // move key code to 'KEYB_IO register'
        }

        int rc = avrvm_exec(&vm);
        if (rc == AVRVM_RC_OK)
            continue;

        if (rc >= 0)
        {
            exec_ext_call(rc);
        }
        else if (rc == AVRVM_RC_UNDEF_INSTR)
        {
            printf("bad instruction");
            exit(-1);
        }
        else if (rc == AVRVM_RC_BREAK)
        {
            printf("end of script");
            exit(0);
        }
        else if (rc == AVRVM_RC_SLEEP)
        {
            printf("sleep request");
        }
    }
}

int main(int argc, const char *argv[])
{
    if (argc < 2) return 0;
    run_hello(argv[1]);

    return 0;
}

