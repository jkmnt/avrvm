#ifndef __AVRVM_H__
#define __AVRVM_H__

#define AVRVM_N_IO_REGS 61
// 32k words, 64kB of flash
#define AVRVM_MAX_FLASH_WORDS 32768
// 32k -- 65k addresses are kind of ROMed external functions

enum avrvm_rc_e
{
    AVRVM_RC_UNDEF_INSTR = -5,
    AVRVM_RC_WDR = -4,
    AVRVM_RC_BREAK = -3,
    AVRVM_RC_SLEEP = -2,
    AVRVM_RC_OK = -1,
    AVRVM_RC_EXT_CALL = 0,
};

typedef struct
{
    uint16_t (*flash_r)(uint16_t word_idx);
    uint8_t (*sram_r)(uint16_t addr);
    void (*sram_w)(uint16_t addr, uint8_t byte);
    uint8_t (*io_r)(uint8_t addr);
    void (*io_w)(uint8_t addr, uint8_t byte);
} avrvm_iface_t;

typedef struct
{
    avrvm_iface_t iface;

    uint16_t pc;
    union
    {
        uint16_t sp;
        struct
        {
            uint8_t spl;
            uint8_t sph;
        };
    };

    uint8_t sreg;

    union {
        uint8_t regs[32];
        uint16_t regsw[16];
    };
} avrvm_ctx_t;

void avrvm_init(avrvm_ctx_t *ctx, const avrvm_iface_t *iface, uint16_t sram_size);
int avrvm_exec(avrvm_ctx_t *ctx);

void avrvm_unpack_args_gcc(const avrvm_ctx_t *ctx, const char *fmt, ...);

#endif
