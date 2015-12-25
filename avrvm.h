#ifndef __AVRVM_H__
#define __AVRVM_H__

#define AVRVM_N_IO_REGS 61

/* 32k words, 64kB of flash are supported for code. Addresses above 32k are treated as a table of ROMed functions */
#define AVRVM_MAX_FLASH_WORDS 32768

enum avrvm_rc_e
{
    /* Failed to decode instruction */
    AVRVM_RC_UNDEF_INSTR = -5,
    /* Watchdog reset. Useful if host is applying strict deadlines to script */
    AVRVM_RC_WDR = -4,
    /* 'break' assembly instruction was decoded. May be treated as a script 'exit' */
    AVRVM_RC_BREAK = -3,
    /* Script got nothing to do and intending to nap until external host event */
    AVRVM_RC_SLEEP = -2,
    /* Usual return value */
    AVRVM_RC_OK = -1,
};

typedef struct
{
    /* called for fetching instruction (or flash data) word. Argument is a word (not byte) offset from the flash base */
    uint16_t (*flash_r)(uint16_t word_idx);
    /* called for reading a byte from sram at the specified address */
    uint8_t (*sram_r)(uint16_t addr);
    /* called for writing a byte to sram at the specified address */
    void (*sram_w)(uint16_t addr, uint8_t byte);
    /* called for reading a byte from the specified i/o register (0 .. 61) */
    uint8_t (*io_r)(uint8_t addr);
    /* called for writing a byte to the specified i/o register (0 .. 61 )*/
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

/*!
 * \brief Init avrvm
 *
 * \param ctx - VM context instance
 * \param iface - iface specification
 * \param sram_size - sram size. Used for setting initial stack pointer only
 */
void avrvm_init(avrvm_ctx_t *ctx, const avrvm_iface_t *iface, uint16_t sram_size);

/*!
 * \brief Execute a single instruction of script
 *
 * \param ctx - VM context instance
 *
 * \return negative return code: one of avrvm_rc_e, non-negative: index of called host API function
 */
int avrvm_exec(avrvm_ctx_t *ctx);

/*!
 * \brief Helper for unpacking function call arguments according to the avr-gcc call convention
 *
 * \param ctx - VM context instance
 * \param fmt - format string specifying ordered arguments
 * \param ... - pointers to variables to store unpacked arguments (similar to scanf)
 *
 * Recognized characters from format string:
 * c - char
 * b - int8_t
 * B - uint8_t
 * h - int16_t
 * H - uint16_t
 * i, l - int32_t
 * I, L - uint32_t
 * f - float (4 byte)
 * q - int64_t
 * Q - uint64_t
 * P - void * pointer in script's memory space, converted to uint16_t offset from the beginning of the host-emulated sram
 */
void avrvm_unpack_args_gcc(const avrvm_ctx_t *ctx, const char *fmt, ...);

/*!
 * \brief Helper for packing function return value according to the avr-gcc call convention
 *
 * \param ctx - VM context instance
 * \param fmt - format character
 * \param val - pointer to variable to return
 *
 * Recognized format characters:
 * c - char
 * b - int8_t
 * B - uint8_t
 * h - int16_t
 * H - uint16_t
 * i, l - int32_t
 * I, L - uint32_t
 * f - float (4 byte)
 * q - int64_t
 * Q - uint64_t
 * P - uint16_t offset from the beginning of the host-emulated sram. converted to void * pointer in script's memory space  
 */
void avrvm_pack_return_gcc(avrvm_ctx_t *ctx, char fmt, const void *val);

#endif
