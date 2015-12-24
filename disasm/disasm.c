#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "avr_op_decoder.h"

static void list_opcode(int address, uint16_t opcode)
{
    static int prev_opcode = -1;

    avr_operands_t ops;
    memset(&ops, 0x55, sizeof(ops));

    if (prev_opcode < 0)
    {
        avr_instr_t instr =  avr_decode(&ops, opcode);
        if (instr == AVR_OP_STS || instr == AVR_OP_LDS || instr == AVR_OP_CALL || instr == AVR_OP_JMP)
        {
            prev_opcode = opcode;
        }
        else
        {
            if (instr == AVR_OP_UNDEFINED
                || instr == AVR_OP_LD_X || instr == AVR_OP_LD_X_DEC || instr == AVR_OP_LD_X_INC
                || instr == AVR_OP_LDD_Y || instr == AVR_OP_LD_Y_DEC || instr == AVR_OP_LD_Y_INC
                || instr == AVR_OP_LDD_Z || instr == AVR_OP_LD_Z_DEC || instr == AVR_OP_LD_Z_INC
                || instr == AVR_OP_ST_X || instr == AVR_OP_ST_X_DEC || instr == AVR_OP_ST_X_INC
                || instr == AVR_OP_STD_Y || instr == AVR_OP_ST_Y_DEC || instr == AVR_OP_ST_Y_INC
                || instr == AVR_OP_STD_Z || instr == AVR_OP_ST_Z_DEC || instr == AVR_OP_ST_Z_INC
                || instr == AVR_OP_LPM_R0 || instr == AVR_OP_LPM_INC)
            {
                printf("%8x:\t%02x %02x       \t", address, opcode & 0xff, opcode >> 8);
            }
            else
            {
                printf("%8x:\t%02x %02x       \t%s\t", address, opcode & 0xff, opcode >> 8, avr_get_instr_name_by_id(instr));
            }

            switch (instr)
            {
            case AVR_OP_RJMP:
            case AVR_OP_RCALL:
                {
                    int offset = ops.pc_offset * 2;
                    printf(".%+d\t;  0x%x", offset, address + offset + 2);
                }
                break;

            case AVR_OP_BRBC:
            case AVR_OP_BRBS:
                {
                    int offset = ops.pc_offset * 2;
                    printf(".%+d\t;  0x%x, bit %d", offset, address + offset + 2, ops.bit);
                }
                break;

            case AVR_OP_SBRS:
            case AVR_OP_SBRC:
                printf("r%d, %d", ops.rr, ops.bit);
                break;

            case AVR_OP_AND:
            case AVR_OP_OR:
            case AVR_OP_EOR:
            case AVR_OP_MOV:
            case AVR_OP_ADD:
            case AVR_OP_ADC:
            case AVR_OP_SUB:
            case AVR_OP_SBC:
            case AVR_OP_CP:
            case AVR_OP_CPC:
            case AVR_OP_CPSE:
                printf("r%d, r%d", ops.rd, ops.rr);
                break;

            case AVR_OP_MUL:
            case AVR_OP_MULS:
            case AVR_OP_MULSU:
            case AVR_OP_FMUL:
            case AVR_OP_FMULS:
            case AVR_OP_FMULSU:
                printf("r%d, r%d", ops.rd, ops.rr);
                break;

            case AVR_OP_MOVW:
                // XXX: rd, rr are word indexes here
                printf("r%d, r%d", ops.rd * 2, ops.rr * 2);
                break;

            case AVR_OP_ADIW:
            case AVR_OP_SBIW:
                // rd is a top word index
                printf("r%d, 0x%02x\t; %d", ops.rd * 2, ops.imm, ops.imm);
                break;

            case AVR_OP_ORI:
            case AVR_OP_ANDI:
            case AVR_OP_CPI:
            case AVR_OP_SUBI:
            case AVR_OP_SBCI:
            case AVR_OP_LDI:
                printf("r%d, 0x%02X\t; %d", ops.rd, ops.imm, ops.imm);
                break;

            case AVR_OP_IN:
                printf("r%d, 0x%02x\t; %d", ops.rd, ops.port, ops.port);
                break;

            case AVR_OP_OUT:
                printf("0x%02x, r%d\t; %d", ops.port, ops.rr, ops.port);
                break;

            case AVR_OP_BLD:
            case AVR_OP_BST:
                // XXX: bst uses rd, not rr !
                printf("r%d, %u", ops.rd, ops.bit);
                break;

            case AVR_OP_CBI:
            case AVR_OP_SBI:
            case AVR_OP_SBIC:
            case AVR_OP_SBIS:
                printf("0x%02x, %d\t; %d", ops.port, ops.bit, ops.port);
                break;

            case AVR_OP_PUSH:
                printf("r%d", ops.rr);
                break;

            case AVR_OP_ASR:
            case AVR_OP_COM:
            case AVR_OP_DEC:
            case AVR_OP_INC:
            case AVR_OP_LSR:
            case AVR_OP_NEG:
            case AVR_OP_POP:
            case AVR_OP_ROR:
            case AVR_OP_SWAP:
                printf("r%d", ops.rd);
                break;

            case AVR_OP_LD_X:
                printf("ld\tr%d, X", ops.rd);
                break;

            case AVR_OP_LD_X_INC:
                printf("ld\tr%d, X+", ops.rd);
                if (ops.rd == 26 || ops.rd == 27) printf("; undefined");
                break;

            case AVR_OP_LD_X_DEC:
                printf("ld\tr%d, -X", ops.rd);
                if (ops.rd == 26 || ops.rd == 27) printf("; undefined");
                break;

            case AVR_OP_LDD_Y:
                if (! ops.ptr_displacement)
                    printf("ld\tr%d, Y", ops.rd);
                else
                    printf("ldd\tr%d, Y+%u\t; 0x%02x", ops.rd, ops.ptr_displacement, ops.ptr_displacement);
                break;

            case AVR_OP_LD_Y_INC:
                printf("ld\tr%d, Y+", ops.rd);
                if (ops.rd == 28 || ops.rd == 29) printf("; undefined");
                break;

            case AVR_OP_LD_Y_DEC:
                printf("ld\tr%d, -Y", ops.rd);
                if (ops.rd == 28 || ops.rd == 29) printf("; undefined");
                break;

            case AVR_OP_LDD_Z:
                if (! ops.ptr_displacement)
                    printf("ld\tr%d, Z", ops.rd);
                else
                    printf("ldd\tr%d, Z+%u\t; 0x%02x", ops.rd, ops.ptr_displacement, ops.ptr_displacement);
                break;

            case AVR_OP_LD_Z_INC:
                printf("ld\tr%d, Z+", ops.rd);
                if (ops.rd == 30 || ops.rd == 31) printf("; undefined");
                break;

            case AVR_OP_LD_Z_DEC:
                printf("ld\tr%d, -Z", ops.rd);
                if (ops.rd == 30 || ops.rd == 31) printf("; undefined");
                break;

///
            case AVR_OP_ST_X:
                printf("st\tX, r%d", ops.rr);
                break;

            case AVR_OP_ST_X_INC:
                printf("st\tX+, r%d", ops.rr);
                if (ops.rr == 26 || ops.rr == 27) printf("; undefined");
                break;

            case AVR_OP_ST_X_DEC:
                printf("st\t-X, r%d", ops.rr);
                if (ops.rr == 26 || ops.rr == 27) printf("; undefined");
                break;

            case AVR_OP_STD_Y:
                if (! ops.ptr_displacement)
                    printf("st\tY, r%d", ops.rr);
                else
                    printf("std\tY+%u, r%d\t; 0x%02x", ops.ptr_displacement, ops.rr, ops.ptr_displacement);
                break;

            case AVR_OP_ST_Y_INC:
                printf("st\tY+, r%d", ops.rr);
                if (ops.rr == 28 || ops.rr == 29) printf("; undefined");
                break;

            case AVR_OP_ST_Y_DEC:
                printf("st\t-Y, r%d", ops.rr);
                if (ops.rr == 28 || ops.rr == 29) printf("; undefined");
                break;

            case AVR_OP_STD_Z:
                if (! ops.ptr_displacement)
                    printf("st\tZ, r%d", ops.rr);
                else
                    printf("std\tZ+%u, r%d\t; 0x%02x", ops.ptr_displacement, ops.rr, ops.ptr_displacement);
                break;

            case AVR_OP_ST_Z_INC:
                printf("st\tZ+, r%d", ops.rr);
                if (ops.rr == 30 || ops.rr == 31) printf("; undefined");
                break;

            case AVR_OP_ST_Z_DEC:
                printf("st\t-Z, r%d", ops.rr);
                if (ops.rr == 30 || ops.rr == 31) printf("; undefined");
                break;

            case AVR_OP_LPM_R0:
                printf("lpm");
                break;

            case AVR_OP_LPM:
                printf("r%d, Z", ops.rd);
                break;

            case AVR_OP_LPM_INC:
                printf("lpm\tr%d, Z+", ops.rd);
                break;
//

            case AVR_OP_UNDEFINED:
                printf(".word\t0x%04x  ; ????", opcode);
                break;
            }

            printf("\n");
        }
    }
    else
    {
        avr_instr_t instr =  avr_decode(&ops, prev_opcode);

        printf("%8x:\t%02x %02x %02x %02x    \t%s\t", address - 2, prev_opcode & 0xff, prev_opcode >> 8, opcode & 0xff, opcode >> 8, avr_get_instr_name_by_id(instr));

        switch (instr)
        {
        case AVR_OP_LDS:
            printf("r%d, 0x%04X", ops.rd, opcode);
            break;

        case AVR_OP_STS:
            printf("0x%04X, r%d", opcode, ops.rr);
            break;

        case AVR_OP_CALL:
        case AVR_OP_JMP:
            {
                uint32_t addr = ((uint32_t) ops.addr_msbyte << 16) | opcode;
                addr *= 2;  // word-based
                printf("0x%x ;   0x%x", addr, addr);
            }
            break;
        }

        printf("\n");

        prev_opcode = -1;
    }
}

int main(int argc, const char *argv[])
{
    FILE *f;

    if (argc < 2) return 0;

    f = fopen(argv[1], "rb");
    if (!f)
    {
        printf("failed to open");
        return -1;
    }

    int addr = 0;
    while (1)
    {
        uint16_t opcode;
        if (fread ( &opcode, 2, 1, f) != 1)
            break;
        list_opcode(addr, opcode);
        addr += 2;
    }

    return 0;
}
