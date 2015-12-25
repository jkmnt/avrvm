#define __AVRVM_C__

#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include "avr_op_decoder.h"
#include "avrvm.h"

#if DEBUG_AVRVM
    #include <stdarg.h>
    #include <stdio.h>
    #define DBG(a, ...) printf(a, __VA_ARGS__)
#else
    #define DBG(a, ...)
#endif

#define F_C   0x01
#define F_Z   0x02
#define F_N   0x04
#define F_V   0x08
#define F_S   0x10
#define F_H   0x20
#define F_T   0x40
#define F_I   0x80

static void writemem(avrvm_ctx_t *ctx, uint16_t addr, uint8_t byte)
{
    if (addr >= 32 + 64)    // sram write (most common operation)
        ctx->iface.sram_w(addr - 32 - 64, byte);
    else if (addr < 32)
        ctx->regs[addr] = byte;
    else if (addr < (32 + 61))
        ctx->iface.io_w(addr - 32, byte);
    else if (addr == (32 + 61)) // spl
        ctx->spl = byte;
    else if (addr == (32 + 62)) // sph
        ctx->sph = byte;
    else // (addr == (32 + 63)) // sreg
        ctx->sreg = byte;
}

static uint8_t readmem(const avrvm_ctx_t *ctx, uint16_t addr)
{
    if (addr >= 32 + 64) // sram read (most common operation)
        return ctx->iface.sram_r(addr - 32 - 64);
    if (addr < 32)
        return ctx->regs[addr];
    if (addr < (32 + 61))
        return ctx->iface.io_r(addr - 32);
    if (addr == (32 + 61)) // spl
        return ctx->spl;
    if (addr == (32 + 62)) // sph
        return ctx->sph;
    //if (addr == (32 + 63)) // sreg
    return ctx->sreg;
}

static uint16_t skip_instr(avrvm_ctx_t *ctx, uint16_t pc)
{
    avr_instr_t instr = avr_decode(NULL, ctx->iface.flash_r(pc));
    if (instr == AVR_OP_STS || instr == AVR_OP_LDS || instr == AVR_OP_CALL || instr == AVR_OP_JMP)
        return pc + 2;
    return pc + 1;
}

static void push(avrvm_ctx_t *ctx, uint8_t byte)
{
    writemem(ctx, ctx->sp--, byte);
}

static uint8_t pop(avrvm_ctx_t *ctx)
{
    return readmem(ctx, ++ctx->sp);
}

static void pushw(avrvm_ctx_t *ctx, uint16_t word)
{
    push(ctx, word >> 8);
    push(ctx, word);
}

static uint16_t popw(avrvm_ctx_t *ctx)
{
    uint16_t word;
    word = pop(ctx);
    word |= pop(ctx) << 8;
    return word;
}

static void update_sreg_after_logic(avrvm_ctx_t *ctx, unsigned int res)
{
    unsigned int sreg = ctx->sreg & ~(F_S | F_V | F_N | F_Z);
    if (! (res & 0xFFFF)) sreg |= F_Z;                         // Z
    sreg |= ((res >> 7) & 0x01) << 2 ;                         // N
                                                               // V cleared
    sreg |= (((sreg >> 2) ^ (sreg >> 3)) & 0x01) << 4;         // S
    ctx->sreg = sreg;
}

static void update_sreg_after_add(avrvm_ctx_t *ctx, unsigned int res, unsigned int rd, unsigned int rr)
{
    unsigned int sreg = ctx->sreg & ~(F_H | F_S | F_V | F_N | F_Z | F_C);
    sreg |= ((res >> 8) & 0x01) << 0;                          // C
    if (! (res & 0xFF)) sreg |= F_Z;                           // Z
    sreg |= ((res >> 7) & 0x01) << 2 ;                         // N
    sreg |= ((((res ^ rd) & (res ^ rr)) >> 7) & 0x01) << 3;    // V
    sreg |= (((sreg >> 2) ^ (sreg >> 3)) & 0x01) << 4;         // S
    sreg |= (((res ^ rd ^ rr) >> 4) & 0x01) << 5;              // H
    ctx->sreg = sreg;
}

static void update_sreg_after_sub(avrvm_ctx_t *ctx, unsigned int res, unsigned int rd, unsigned int rr)
{
    unsigned int sreg = ctx->sreg & ~(F_H | F_S | F_V | F_N | F_C);
    sreg |= ((res >> 8) & 0x01) << 0;                           // C
    if (res & 0xFF) sreg &= ~F_Z;                               // Z (only clear)
    sreg |= ((res >> 7) & 0x01) << 2 ;                          // N
    sreg |= ((((rr ^ rd) & (res ^ rd)) >> 7) & 0x01) << 3;      // V
    sreg |= (((sreg >> 2) ^ (sreg >> 3)) & 0x01) << 4;          // S
    sreg |= (((res ^ rd ^ rr) >> 4) & 0x01) << 5;               // H
    ctx->sreg = sreg;
}

static void update_sreg_after_mul(avrvm_ctx_t *ctx, unsigned int res)
{
    unsigned int sreg = ctx->sreg & ~(F_Z | F_C);
    sreg |= ((res >> 15) & 0x01) << 0;  // C
    if (! (res & 0xFFFF)) sreg |= F_Z;
    ctx->sreg = sreg;
}

static void update_sreg_after_fmul(avrvm_ctx_t *ctx, unsigned int res)
{
    unsigned int sreg = ctx->sreg & ~(F_Z | F_C);
    sreg |= ((res >> 16) & 0x01) << 0;  // C
    if (! (res & 0xFFFF)) sreg |= F_Z;
    ctx->sreg = sreg;
}

static void update_sreg_after_rshift(avrvm_ctx_t *ctx, unsigned int res, unsigned int rd)
{
    unsigned int sreg = ctx->sreg & ~(F_S | F_V | F_N | F_Z | F_C);
    sreg |= rd & 0x01;                                  // C = lsb before shift
    if (! (res & 0xFF)) sreg |= F_Z;                    // Z
    sreg |= ((res >> 7) & 0x01) << 2 ;                  // N
    sreg |= (((sreg >> 2) ^ (sreg >> 0)) & 0x01) << 3;  // V = N^C
    sreg |= (((sreg >> 2) ^ (sreg >> 3)) & 0x01) << 4;  // S = N^V
    ctx->sreg = sreg;
}

int avrvm_exec(avrvm_ctx_t *ctx)
{
    avr_operands_t ops;

#define RD (ctx->regs[ops.rd])
#define RR (ctx->regs[ops.rr])
#define RDW (ctx->regsw[ops.rd])
#define RRW (ctx->regsw[ops.rr])

#define R0R1 (ctx->regsw[0])
#define X (ctx->regsw[13])
#define Y (ctx->regsw[14])
#define Z (ctx->regsw[15])

    uint16_t opcode = ctx->iface.flash_r(ctx->pc);
    ctx->pc += 1;
    avr_instr_t instr =  avr_decode(&ops, opcode);

    unsigned int res;
    unsigned int sreg;

    switch (instr)
    {
//--- arithmetic and logic
    case AVR_OP_ADD:
        DBG("add a:%x b:%x ", RD, RR);
        res = RD + RR;
        update_sreg_after_add(ctx, res, RD, RR);
        RD = res;
        DBG("r:%x s:%x\n", RD, ctx->sreg);
        break;

    case AVR_OP_ADC:
        DBG("adc a:%x b:%x s0:%x ", RD, RR, ctx->sreg);
        res = RD + RR + (ctx->sreg & F_C);
        update_sreg_after_add(ctx, res, RD, RR);
        RD = res;
        DBG("r:%x s:%x\n", RD, ctx->sreg);
        break;

    //XXX: untested
    case AVR_OP_ADIW:
        res = RDW + ops.imm;
        sreg = ctx->sreg & ~(F_S | F_V | F_N | F_Z | F_C);
        sreg |= ((res >> 16) & 0x01) << 0;                         // C
        if (! (res & 0xFFFF)) sreg |= F_Z;                         // Z
        sreg |= ((res >> 15) & 0x01) << 2 ;                        // N
        sreg |= (((~RDW & res) >> 15) & 0x01) << 3;                // V
        sreg |= (((sreg >> 2) ^ (sreg >> 3)) & 0x01) << 4;         // S = N^V
        ctx->sreg = sreg;
        RDW = res;
        break;

    case AVR_OP_SUB:
        DBG("sub a:%x b:%x ", RD, RR);
        res = RD - RR;
        ctx->sreg |= F_Z;    //NOTE: hack to reuse same logic for sub/sbc
        update_sreg_after_sub(ctx, res, RD, RR);
        RD = res;
        DBG("r:%x s:%x\n", RD, ctx->sreg);
        break;

    case AVR_OP_SUBI:
        res = RD - ops.imm;
        ctx->sreg |= F_Z;    //NOTE: hack to reuse same logic for sub/sbc
        update_sreg_after_sub(ctx, res, RD, ops.imm);
        RD = res;
        break;

    case AVR_OP_SBC:
        DBG("sbc a:%x b:%x s0:%x ", RD, RR, ctx->sreg);
        res = RD - RR - (ctx->sreg & F_C);
        update_sreg_after_sub(ctx, res, RD, RR);
        RD = res;
        DBG("r:%x s:%x\n", RD, ctx->sreg);
        break;

    case AVR_OP_SBCI:
        res = RD - ops.imm - (ctx->sreg & F_C);
        update_sreg_after_sub(ctx, res, RD, ops.imm);
        RD = res;
        break;

    //XXX: untested
    case AVR_OP_SBIW:
        res = RDW - ops.imm;

        sreg = ctx->sreg & ~(F_S | F_V | F_N | F_Z | F_C);
        sreg |= ((res >> 16) & 0x01) << 0;                         // C
        if (! (res & 0xFFFF)) sreg |= F_Z;                         // Z
        sreg |= ((res >> 15) & 0x01) << 2 ;                        // N
        sreg |= (((RDW & ~res) >> 15) & 0x01) << 3;                // V
        sreg |= (((sreg >> 2) ^ (sreg >> 3)) & 0x01) << 4;         // S = N^V
        ctx->sreg = sreg;

        RDW = res;
        break;

    case AVR_OP_AND:
        res = RD & RR;
        update_sreg_after_logic(ctx, res);
        RD = res;
        break;

    case AVR_OP_ANDI:
        res = RD & ops.imm;
        update_sreg_after_logic(ctx, res);
        RD = res;
        break;

    case AVR_OP_OR:
        res = RD | RR;
        update_sreg_after_logic(ctx, res);
        RD = res;
        break;

    case AVR_OP_ORI:
        res = RD | ops.imm;
        update_sreg_after_logic(ctx, res);
        RD = res;
        break;

    case AVR_OP_EOR:
        DBG("eor a:%x b:%x ", RD, RR);
        res = RD ^ RR;
        update_sreg_after_logic(ctx, res);
        RD = res;
        DBG("r:%x s:%x\n", RD, ctx->sreg);
        break;

    case AVR_OP_COM:
        DBG("com a:%x ", RD);
        res = ~RD;
        sreg = ctx->sreg & ~(F_S | F_V | F_N | F_Z | F_C);
        sreg |= F_C;                                                // C set
        if (! (res & 0xFF)) sreg |= F_Z;                            // Z
        sreg |= ((res >> 7) & 0x01) << 2 ;                          // N
                                                                    // V cleared
        sreg |= (((sreg >> 2) ^ (sreg >> 3)) & 0x01) << 4;          // S
        ctx->sreg = sreg;
        RD = res;
        DBG("r:%x s:%x\n", RD, ctx->sreg);
        break;

    case AVR_OP_NEG:
        DBG("neg a:%x ", RD);
        res = -RD;
        ctx->sreg |= F_Z;
        update_sreg_after_sub(ctx, res, 0, RD);
        RD = res;
        DBG("r:%x s:%x\n", RD, ctx->sreg);
        break;

    case AVR_OP_INC:
        DBG("inc a:%x ", RD);
        res = RD + 1;
        sreg = ctx->sreg & ~(F_S | F_V | F_N | F_Z);
        if (! (res & 0xFF)) sreg |= F_Z;                            // Z
        sreg |= ((res >> 7) & 0x01) << 2 ;                          // N
        if (RD == 0x7F) sreg |= F_V;                                // V
        sreg |= (((sreg >> 2) ^ (sreg >> 3)) & 0x01) << 4;          // S
        ctx->sreg = sreg;
        RD = res;
        DBG("r:%x s:%x\n", RD, ctx->sreg);
        break;

    case AVR_OP_DEC:
        DBG("dec a:%x ", RD);
        res = RD - 1;
        sreg = ctx->sreg & ~(F_S | F_V | F_N | F_Z);
        if (! (res & 0xFF)) sreg |= F_Z;                            // Z
        sreg |= ((res >> 7) & 0x01) << 2 ;                          // N
        if (RD == 0x80) sreg |= F_V;                                // V
        sreg |= (((sreg >> 2) ^ (sreg >> 3)) & 0x01) << 4;          // S
        ctx->sreg = sreg;
        RD = res;
        DBG("r:%x s:%x\n", RD, ctx->sreg);
        break;

    case AVR_OP_MUL:
        DBG("mul a:%x b:%x ", RD, RR);
        res = RD * RR;
        update_sreg_after_mul(ctx, res);
        R0R1 = res;
        DBG("r:%x s:%x\n", R0R1, ctx->sreg);
        break;

    case AVR_OP_MULS:
        DBG("muls a:%x b:%x ", RD, RR);
        res = (int8_t)RD * (int8_t)RR;
        update_sreg_after_mul(ctx, res);
        R0R1 = res;
        DBG("r:%x s:%x\n", R0R1, ctx->sreg);
        break;

    case AVR_OP_MULSU:
        DBG("mulsu a:%x b:%x ", RD, RR);
        res = (int8_t)RD * RR;
        update_sreg_after_mul(ctx, res);
        R0R1 = res;
        DBG("r:%x s:%x\n", R0R1, ctx->sreg);
        break;


    case AVR_OP_FMUL:
        DBG("fmul a:%x b:%x ", RD, RR);
        res = (RD * RR) << 1;
        update_sreg_after_fmul(ctx, res);
        R0R1 = res;
        DBG("r:%x s:%x\n", R0R1, ctx->sreg);
        break;

    case AVR_OP_FMULS:
        DBG("fmuls a:%x b:%x ", RD, RR);
        res = ((int8_t)RD * (int8_t)RR) << 1;
        update_sreg_after_fmul(ctx, res);
        R0R1 = res;
        DBG("r:%x s:%x\n", R0R1, ctx->sreg);
        break;

    case AVR_OP_FMULSU:
        DBG("fmulsu a:%x b:%x ", RD, RR);
        res = ((int8_t)RD * RR) << 1;
        update_sreg_after_fmul(ctx, res);
        R0R1 = res;
        DBG("r:%x s:%x\n", R0R1, ctx->sreg);
        break;

//--- branch
    case AVR_OP_RJMP:
        ctx->pc += ops.pc_offset;
        break;

    case AVR_OP_IJMP:
        ctx->pc = Z;
        break;

    case AVR_OP_JMP:
        ctx->pc = (ctx->iface.flash_r(ctx->pc) | ((uint32_t) ops.addr_msbyte << 16));
        break;

    // NOTE: rcall won't reach ROM calls, that's by design
    case AVR_OP_RCALL:
        pushw(ctx, ctx->pc);
        ctx->pc += ops.pc_offset;
        break;

    case AVR_OP_ICALL:
        if (Z < AVRVM_MAX_FLASH_WORDS)
        {
            pushw(ctx, ctx->pc);
            ctx->pc = Z;
        }
        else
        {
            return Z - AVRVM_MAX_FLASH_WORDS;   // ROM call, return func 'index'
        }
        break;

    case AVR_OP_CALL:
        {
            uint16_t addr = (ctx->iface.flash_r(ctx->pc) | ((uint32_t) ops.addr_msbyte << 16));
            ctx->pc += 1;   // skip data word
            if (addr < AVRVM_MAX_FLASH_WORDS)
            {
                pushw(ctx, ctx->pc);
                ctx->pc = addr;
            }
            else
            {
                return Z - AVRVM_MAX_FLASH_WORDS;   // ROM call, return func 'index'
            }
        }
        break;

    case AVR_OP_RET:
        ctx->pc = popw(ctx);
        break;

    case AVR_OP_CPSE:
        if (RD == RR)
            ctx->pc = skip_instr(ctx, ctx->pc);
        break;

    case AVR_OP_CP:
        res = RD - RR;
        ctx->sreg |= F_Z;    //NOTE: hack to reuse same logic for sub/sbc
        update_sreg_after_sub(ctx, res, RD, RR);
        break;

    case AVR_OP_CPI:
        res = RD - ops.imm;
        ctx->sreg |= F_Z;    //NOTE: hack to reuse same logic for sub/sbc
        update_sreg_after_sub(ctx, res, RD, ops.imm);
        break;

    case AVR_OP_CPC:
        res = RD - RR - (ctx->sreg & F_C);
        update_sreg_after_sub(ctx, res, RD, RR);
        break;

    case AVR_OP_SBRS:
        if (RR & (1 << ops.bit))
            ctx->pc = skip_instr(ctx, ctx->pc);
        break;

    case AVR_OP_SBRC:
        if (! (RR & (1 << ops.bit)))
            ctx->pc = skip_instr(ctx, ctx->pc);
        break;

    case AVR_OP_SBIS:
        if (readmem(ctx, ops.port + 32) & (1 << ops.bit))
            ctx->pc = skip_instr(ctx, ctx->pc);
        break;

    case AVR_OP_SBIC:
        if (! (readmem(ctx, ops.port + 32) & (1 << ops.bit)))
            ctx->pc = skip_instr(ctx, ctx->pc);
        break;

    case AVR_OP_BRBC:
        if (! (ctx->sreg & (1 << ops.bit)))
            ctx->pc += ops.pc_offset;
        break;

    case AVR_OP_BRBS:
        if (ctx->sreg & (1 << ops.bit))
            ctx->pc += ops.pc_offset;
        break;

//--- data transfer
    case AVR_OP_MOV:
        RD = RR;
        break;

    case AVR_OP_MOVW:
        RDW = RRW;
        break;

    case AVR_OP_LDI:
        RD = ops.imm;
        break;

    case AVR_OP_LDS:
        RD = readmem(ctx, ctx->iface.flash_r(ctx->pc++));
        break;

    case AVR_OP_LD_X:
        RD = readmem(ctx, X);
        break;

    case AVR_OP_LD_X_INC:
        RD = readmem(ctx, X++);
        break;

    case AVR_OP_LD_X_DEC:
        RD = readmem(ctx, --X);
        break;

    case AVR_OP_LDD_Y:
        RD = readmem(ctx, Y + ops.ptr_displacement);
        break;

    case AVR_OP_LD_Y_INC:
        RD = readmem(ctx, Y++);
        break;

    case AVR_OP_LD_Y_DEC:
        RD = readmem(ctx, --Y);
        break;

    case AVR_OP_LDD_Z:
        RD = readmem(ctx, Z + ops.ptr_displacement);
        break;

    case AVR_OP_LD_Z_INC:
        RD = readmem(ctx, Z++);
        break;

    case AVR_OP_LD_Z_DEC:
        RD = readmem(ctx, --Z);
        break;

    case AVR_OP_STS:
        writemem(ctx, ctx->iface.flash_r(ctx->pc++), RR);
        break;

    case AVR_OP_ST_X:
        writemem(ctx, X, RR);
        break;

    case AVR_OP_ST_X_INC:
        writemem(ctx, X++, RR);
        break;

    case AVR_OP_ST_X_DEC:
        writemem(ctx, --X, RR);
        break;

    case AVR_OP_STD_Y:
        writemem(ctx, Y + ops.ptr_displacement, RR);
        break;

    case AVR_OP_ST_Y_INC:
        writemem(ctx, Y++, RR);
        break;

    case AVR_OP_ST_Y_DEC:
        writemem(ctx, --Y, RR);
        break;

    case AVR_OP_STD_Z:
        writemem(ctx, Z + ops.ptr_displacement, RR);
        break;

    case AVR_OP_ST_Z_INC:
        writemem(ctx, Z++, RR);
        break;

    case AVR_OP_ST_Z_DEC:
        writemem(ctx, --Z, RR);
        break;

    case AVR_OP_LPM:
        RD = ctx->iface.flash_r(Z / 2) >> (Z & 0x01 ? 8 : 0);
        break;

    case AVR_OP_LPM_INC:
        RD = ctx->iface.flash_r(Z / 2) >> (Z & 0x01 ? 8 : 0);
        Z += 1;
        break;

    case AVR_OP_LPM_R0:
        ctx->regs[0] = ctx->iface.flash_r(Z / 2) >> (Z & 0x01 ? 8 : 0);
        break;

    case AVR_OP_IN:
        RD = readmem(ctx, ops.port + 32);
        break;

    case AVR_OP_OUT:
        writemem(ctx, ops.port + 32, RR);
        break;

    case AVR_OP_PUSH:
        push(ctx, RR);
        break;

    case AVR_OP_POP:
        RD = pop(ctx);
        break;

//--- bit and bit-test
    case AVR_OP_LSR:
        DBG("lsr a:%x s0:%x ", RD, ctx->sreg);
        res = RD >> 1;
        update_sreg_after_rshift(ctx, res, RD);
        RD = res;
        DBG("r:%x s:%x\n", RD, ctx->sreg);
        break;

    case AVR_OP_ROR:
        DBG("ror a:%x s0:%x ", RD, ctx->sreg);
        res = (RD >> 1) | ((ctx->sreg & F_C) << 7);
        update_sreg_after_rshift(ctx, res, RD);
        RD = res;
        DBG("r:%x s:%x\n", RD, ctx->sreg);
        break;

    case AVR_OP_ASR:
        DBG("asr a:%x s0:%x ", RD, ctx->sreg);
        res = ((int8_t) RD) >> 1;
        update_sreg_after_rshift(ctx, res, RD);
        RD = res;
        DBG("r:%x s:%x\n", RD, ctx->sreg);
        break;

    case AVR_OP_SWAP:
        res = (RD << 4) | (RD >> 4);
        RD = res;
        break;

    case AVR_OP_BSET:
        ctx->sreg |= (1 << ops.bit);
        break;

    case AVR_OP_BCLR:
        ctx->sreg &= ~(1 << ops.bit);
        break;

    case AVR_OP_SBI:
        writemem(ctx, ops.port + 32, readmem(ctx, ops.port + 32) | (1 << ops.bit));
        break;

    case AVR_OP_CBI:
        writemem(ctx, ops.port + 32, readmem(ctx, ops.port + 32) | ~(1 << ops.bit));
        break;

    case AVR_OP_BST:
        if (RD & (1 << ops.bit))
            ctx->sreg |= F_T;
        else
            ctx->sreg &= ~F_T;
        break;

    case AVR_OP_BLD:
        if (ctx->sreg & F_T)
            RD |= (1 << ops.bit);
        else
            RD &= ~(1 << ops.bit);
        break;

    case AVR_OP_NOP:
        break;

//--- special returns
    case AVR_OP_WDR:
        return AVRVM_RC_WDR;

    case AVR_OP_SLEEP:
        return AVRVM_RC_SLEEP;

    case AVR_OP_BREAK:
        return AVRVM_RC_BREAK;

    case AVR_OP_UNDEFINED:
        return AVRVM_RC_UNDEF_INSTR;
    }

    return AVRVM_RC_OK;
}

void avrvm_init(avrvm_ctx_t *ctx, const avrvm_iface_t *iface, uint16_t sram_size)
{
    memset(ctx, 0, sizeof(avrvm_ctx_t));

    ctx->pc = 0;
    ctx->sp = 32 + 64 + sram_size - 1;
    ctx->iface = *iface;
}

void avrvm_unpack_args_gcc(const avrvm_ctx_t *ctx, const char *fmt, ...)
{
//  from avr-libc:
//  char is 8 bits, int is 16 bits, long is 32 bits, long long is 64 bits, float and
//  double are 32 bits (this is the only supported floating point format), pointers are 16 bits
//
//  Arguments - allocated left to right, r25 to r8. All arguments are aligned to start in
//  even-numbered registers (odd-sized arguments, including char, have one free
//  register above them).
//  If too many, those that don’t fit are passed on the stack

    va_list vl;
    va_start(vl, fmt);

    int idx = 26; // r25 + 1
    int are_using_stack = 0;

    for (const char *p = fmt; *p; p++)
    {
        int size;

        switch (*p)
        {
        case 'c':   // char     (1)
        case 'b':   // int8_t   (1)
        case 'B':   // uint8_t  (1)
            size = 1;
            break;
        case 'h':   // int16_t  (2)
        case 'H':   // uint16_t (2)
        case 'P':   // void *   (2) -> pointer arg is special, see below
            size = 2;
            break;
        case 'i':   // int32_t  (4)
        case 'I':   // uint32_t (4)
        case 'l':   // int32_t  (4)
        case 'L':   // uint32_t (4)
        case 'f':   // float    (4)
            size = 4;
            break;
        case 'q':   // int64_t  (8)
        case 'Q':   // uint64_t (8)
            size = 8;
            break;
        default:
            while (1);  // instead of heavyweight assert
            break;
        }

        void *arg = va_arg(vl, void *);

        if (! are_using_stack)
        {
            idx -= (size + 1) & -2; // ceil to nearest even
            if (idx >= 8)   // r8
            {
                memcpy(arg, &ctx->regs[idx], size);
            }
            else    // won't fit, switch to stack
            {
                are_using_stack = 1;
                idx = 0;
            }
        }

        if (are_using_stack)
        {
            for (int i = 0; i < size; i++)
                *((uint8_t *)arg + i) = readmem(ctx, ctx->sp + 1 + idx + i);
            idx += size;
        }

        if (*p == 'P')  // pointer arg is special - patch result to convert it to the sram offset
            *((uint16_t *)arg) -= 32 + 64;
    }
    va_end(vl);
}

//  from avr-libc:
//Return values: 8-bit in r24 (not r25!), 16-bit in r25:r24, up to 32 bits in r22-r25, up to
//64 bits in r18-r25. 8-bit return values are zero/sign-extended to 16 bits by the called
//function
void avrvm_pack_return_gcc(avrvm_ctx_t *ctx, char fmt, const void *val)
{
    switch (fmt)
    {
    case 'c':   // char     (1)
        ctx->regsw[12] = *(const char *)val;
        break;
    case 'b':   // int8_t   (1)
        ctx->regsw[12] = *(const int8_t *)val;
        break;
    case 'B':   // uint8_t  (1)
        ctx->regsw[12] = *(const uint8_t *)val;
        break;
    case 'h':   // int16_t  (2)
    case 'H':   // uint16_t (2)
        ctx->regsw[12] = *(const uint16_t *)val;
        break;
    case 'i':   // int32_t  (4)
    case 'I':   // uint32_t (4)
    case 'l':   // int32_t  (4)
    case 'L':   // uint32_t (4)
    case 'f':   // float    (4)
        memcpy(&ctx->regs[22], val, 4);
        break;
    case 'q':   // int64_t  (8)
    case 'Q':   // uint64_t (8)
        memcpy(&ctx->regs[18], val, 8);
        break;
    case 'P':   // void *   (2) -> pointer arg is special, convert from sram offset to mem pointer
        ctx->regsw[12] = *(const uint16_t *)val + 32 + 64;
        break;
    default:
        while (1);
        break;
    }
}
