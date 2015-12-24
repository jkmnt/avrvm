#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "avrvm.h"


static FILE *testfile;

static avrvm_ctx_t test_ctx;
static uint16_t fetch_op(uint16_t word_idx);

static uint8_t sram[1024 + 160];

static uint8_t sram_reader(uint16_t addr)
{
//  printf("<-SRAM[%d]\n", addr);
    if (addr < sizeof(sram))
        return sram[addr];
    return 0;
}

static void sram_writer(uint16_t addr, uint8_t byte)
{
//  printf("->SRAM[%d]\n", addr);
    if (addr < sizeof(sram))
        sram[addr] = byte;
}

static uint8_t io_reader(uint8_t addr)
{
    return 0;
}

static void io_writer(uint8_t addr, uint8_t byte)
{
    return;
}

void run_bin(const char *fname)
{
    avrvm_iface_t iface =
    {
        .flash_r = fetch_op,
        .sram_r = sram_reader,
        .sram_w = sram_writer,
        .io_r = io_reader,
        .io_w = io_writer,
    };

    avrvm_init(&test_ctx, &iface, sizeof(sram));
    testfile = fopen(fname, "rb");
    if (!testfile)
    {
        printf("failed to open");
        return;
    }

    while (1)
    {
        int rc = avrvm_exec(&test_ctx);
        if (rc == AVRVM_RC_OK)
            continue;

        if (rc >= 0)
        {
            printf("ext call %d !", rc);
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

void stop(void)
{

}

uint16_t fetch_op(uint16_t word_idx)
{
    // wrap stuff
    word_idx &= 0x0FFF;
//  printf("\tfetch 0x%04x\n", word_idx);
    if (fseek(testfile, word_idx * 2, SEEK_SET) != 0)
    {
        printf("failed to seek");
        exit(-1);
    }

    uint16_t opcode;
    if (fread ( &opcode, 2, 1, testfile) != 1)
    {
        printf("failed to read");
        exit(-1);
    }

    return opcode;
}


int main(int argc, const char *argv[])
{
    if (argc < 2) return 0;
    run_bin(argv[1]);

    return 0;
}
