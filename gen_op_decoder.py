# python
# quick and dirty avr vm opcode parser generator :-)
#
import re
import jinja2
import pickle

# derived from spec at https://sourceware.org/binutils/docs/as/AVR-Opcodes.html
#       r   source register
#       R   source upper register (r16-r31)
#       d   destination register
#       D   destination upper register (r16-r31)
#       v   `movw' even register (r0, r2, ..., r28, r30)
#       a   `fmul' register (r16-r23)
#       w   `adiw' register (r24,r26,r28,r30)
#       e   pointer registers (X,Y,Z)
#       b   base pointer register and displacement ([YZ]+disp)
#       z   Z pointer register (for [e]lpm Rd,Z[+])
#       M   immediate value from 0 to 255
#       n   immediate value from 0 to 255 ( n = ~M ). Relocation impossible
#       s   immediate value from 0 to 7
#       P   Port address value from 0 to 63. (in, out)
#       p   Port address value from 0 to 31. (cbi, sbi, sbic, sbis)
#       K   immediate value from 0 to 63 (used in `adiw', `sbiw')
#       i   immediate value
#       l   signed pc relative offset from -64 to 63
#       L   signed pc relative offset from -2048 to 2047
#       h   absolute code address (call, jmp)
#       S   immediate value from 0 to 7 (S = s << 4)
#       ?   use this opcode entry if no parameters, else use next opcode entry
#       q   0 to 63 displacement for direct addressing

SPEC = [
    ('1001010010001000','clc'),
    ('1001010011011000','clh'),
    ('1001010010101000','cln'),
    ('1001010011001000','cls'),
    ('1001010011101000','clt'),
    ('1001010010111000','clv'),
    ('1001010010011000','clz'),
    ('1001010011111000','cli'),
    ('1001010000001000','sec'),
    ('1001010001011000','seh'),
    ('1001010000101000','sen'),
    ('1001010001001000','ses'),
    ('1001010001101000','set'),
    ('1001010000111000','sev'),
    ('1001010000011000','sez'),
    ('1001010001111000','sei'),

    ('100101001sss1000','bclr'),       #    S
    ('100101000sss1000','bset'),       #    S
    ('1001010100001001','icall'),      #
    ('1001010000001001','ijmp'),       #
    ('1001010111001000','lpm_r0'),     #
    ('1001000ddddd0100','lpm'),        #
    ('1001000ddddd0101','lpm_inc'),    #
    ('0000000000000000','nop'),        #
    ('1001010110101000','wdr'),        #
    ('1001010100001000','ret'),        #
    ('000111rdddddrrrr','adc'),        #r,r
    ('000011rdddddrrrr','add'),        #r,r
    ('001000rdddddrrrr','and'),        #r,r
    ('000101rdddddrrrr','cp'),         #r,r
    ('000001rdddddrrrr','cpc'),        #r,r
    ('000100rdddddrrrr','cpse'),       #r,r
    ('001001rdddddrrrr','eor'),        #r,r
    ('001011rdddddrrrr','mov'),        #r,r
    ('100111rdddddrrrr','mul'),        #r,r
    ('001010rdddddrrrr','or'),         #r,r
    ('000010rdddddrrrr','sbc'),        #r,r
    ('000110rdddddrrrr','sub'),        #r,r
    ('0111KKKKDDDDKKKK','andi'),       #d,M
    ('1110KKKKDDDDKKKK','ldi'),        #d,M
    ('0110KKKKDDDDKKKK','ori'),        #d,M
    ('0011KKKKDDDDKKKK','cpi'),        #d,M
    ('0100KKKKDDDDKKKK','sbci'),       #d,M
    ('0101KKKKDDDDKKKK','subi'),       #d,M
    ('1111110rrrrr0sss','sbrc'),       #r,s
    ('1111111rrrrr0sss','sbrs'),       #r,s
    ('1111100ddddd0sss','bld'),        #r,s
    ('1111101ddddd0sss','bst'),        #r,s
    ('10110PPdddddPPPP','in'),         #r,P
    ('10111PPrrrrrPPPP','out'),        #P,r
    ('10010110KKwwKKKK','adiw'),       #w,K
    ('10010111KKwwKKKK','sbiw'),       #w,K
    ('10011000pppppsss','cbi'),        #p,s
    ('10011010pppppsss','sbi'),        #p,s
    ('10011001pppppsss','sbic'),       #p,s
    ('10011011pppppsss','sbis'),       #p,s

    ('111101lllllll000','brcc'),       #l
    ('111100lllllll000','brcs'),       #l
    ('111100lllllll001','breq'),       #l
    ('111101lllllll100','brge'),       #l
    ('111101lllllll101','brhc'),       #l
    ('111100lllllll101','brhs'),       #l
    ('111100lllllll100','brlt'),       #l
    ('111100lllllll010','brmi'),       #l
    ('111101lllllll001','brne'),       #l
    ('111101lllllll010','brpl'),       #l
    ('111101lllllll110','brtc'),       #l
    ('111100lllllll110','brts'),       #l
    ('111101lllllll011','brvc'),       #l
    ('111100lllllll011','brvs'),       #l
    ('111101lllllll111','brid'),       #l
    ('111100lllllll111','brie'),       #l

    ('111101lllllllsss','brbc'),       #s,l
    ('111100lllllllsss','brbs'),       #s,l
    ('1101LLLLLLLLLLLL','rcall'),      #L
    ('1100LLLLLLLLLLLL','rjmp'),       #L
    ('1001010hhhhh111h','call'),       #h
    ('1001010hhhhh110h','jmp'),        #h
    ('1001010ddddd0101','asr'),        #r
    ('1001010ddddd0000','com'),        #r
    ('1001010ddddd1010','dec'),        #r
    ('1001010ddddd0011','inc'),        #r
    ('1001010ddddd0110','lsr'),        #r
    ('1001010ddddd0001','neg'),        #r
    ('1001000ddddd1111','pop'),        #r
    ('1001001rrrrr1111','push'),       #r
    ('1001010ddddd0111','ror'),        #r
    ('1001010ddddd0010','swap'),       #r
    ('00000001ddddrrrr','movw'),       #v,v
    ('1001001rrrrr0000','sts'),        #i,r

    ('1001000ddddd0000','lds'),        #r,i

    ('1001000ddddd1100', 'ld_x'),
    ('1001000ddddd1101', 'ld_x_inc'),
    ('1001000ddddd1110', 'ld_x_dec'),

    ('1001000ddddd1001', 'ld_y_inc'),
    ('1001000ddddd1010', 'ld_y_dec'),
    ('10q0qq0ddddd1qqq', 'ldd_y'),

    ('1001000ddddd0001', 'ld_z_inc'),
    ('1001000ddddd0010', 'ld_z_dec'),
    ('10q0qq0ddddd0qqq', 'ldd_z'),

    ('1001001rrrrr1100', 'st_x'),
    ('1001001rrrrr1101', 'st_x_inc'),
    ('1001001rrrrr1110', 'st_x_dec'),

    ('1001001rrrrr1001', 'st_y_inc'),
    ('1001001rrrrr1010', 'st_y_dec'),
    ('10q0qq1rrrrr1qqq', 'std_y'),

    ('1001001rrrrr0001', 'st_z_inc'),
    ('1001001rrrrr0010', 'st_z_dec'),
    ('10q0qq1rrrrr0qqq', 'std_z'),


#   ('10o0oo0dddddbooo','ldd'),        #r,b
#   ('100!000dddddee-+','ld'),         #r,e

#   ('10o0oo1rrrrrbooo','std'),        #b,r
#   ('100!001rrrrree-+','st'),         #e,r

# aliases/macros
#   ('000111rdddddrrrr','rol'),        #r
#   ('0111KKKKddddKKKK','cbr'),        #d,n
#   ('0110KKKKddddKKKK','sbr'),        #d,M
#   ('001001rdddddrrrr','clr'),        #r
#   ('000011rdddddrrrr','lsl'),        #r
#   ('111101lllllll000','brsh'),       #l
#   ('001000rdddddrrrr','tst'),        #r
#   ('111100lllllll000','brlo'),       #l
#   ('11101111dddd1111','ser'),        #d

# useless ?
    ('1001010100011000', 'reti'),
    ('00000010DDDDRRRR','muls'),       #d,d
    ('000000110DDD0RRR','mulsu'),
    ('000000110DDD1RRR','fmul'),       #a,a
    ('000000111DDD0RRR','fmuls'),      #a,a
    ('000000111DDD1RRR','fmulsu'),     #a,a
    ('1001010110001000','sleep'),      #
    ('1001010110011000','break'),
]

MNEMOS = [
    'clc', 'clz', 'cln', 'cls', 'clt', 'clv', 'cli', 'clh',
    'sec', 'sez', 'sen', 'ses', 'set', 'sev', 'sei', 'seh',
    'brcc', 'brcs', 'breq', 'brge', 'brhc', 'brhs', 'brlt', 'brmi',
    'brne', 'brpl', 'brtc', 'brts', 'brvc', 'brvs', 'brid', 'brie',
]

USELESS = [
    'reti',
]

REMOVE_MNEMOS = True
REMOVE_USELESS = True

if REMOVE_MNEMOS:
    SPEC = [s for s in SPEC if s[1] not in MNEMOS]
if REMOVE_USELESS:
    SPEC = [s for s in SPEC if s[1] not in USELESS]

OPERAND_MAP = {
    'd':'rd',
    'D':'rd',
    'w':'rd',
    'r':'rr',
    'R':'rr',
    'l':'pc_offset',
    's':'bit',
    'P':'port',
    'K':'imm',
    'q':'ptr_displacement',
    'p':'port',
    'L':'pc_offset',
    'h':'addr_msbyte',
}

###

def collect_operands(opcode):
    operands = list(set(opcode) - set('10'))
    operands.sort()
    return operands

def gen_operand_unpacker(opcode, op):
    opcode = opcode[::-1]
    groups = []
    total_nbits = 0
    for m in re.finditer('[' + op + ']+', opcode):
        span = m.span()
        nbits = span[1] - span[0]
        mask = ((1 << nbits) - 1) << span[0]
        groups += [{'mask': mask, 'rshift':span[0] - total_nbits}]
        total_nbits += nbits
    return groups

def create_oplist(spec):
    oplist = []
    for opcode, instr in spec:
        operands = collect_operands(opcode)
        entry = {
            'opcode':opcode,
            'instr':instr,
            'operands': operands,
        }
        entry['unpackers'] = {}
        for op in operands:
            entry['unpackers'][op] = gen_operand_unpacker(opcode, op)
        oplist += [entry]
    return oplist

def is_op_match(src, mask):
    for s, m in zip(src, mask):
        if s != m and s in '01' and m in '01':
            return False
    return True

def find_best_nmsbits(oplist):
    # research hashing of msbyte
    def research_hashing(nmsbits):
        hashset = []
        template = '{0:0%db}%s' % (nmsbits, ' ' * (16 - nmsbits))
        for i in range(1 << nmsbits):
            strmask = template.format(i)
            matches = []
            for op in oplist:
                if is_op_match(op['opcode'], strmask):
                    matches += [op['instr']]
            if not matches:
                matches += ['undefined']
            matches = ' '.join(matches)
            if matches not in hashset:
                hashset += [matches]
        return hashset

    hashsets = []
    for nmsbits in range(2, 15):
        hashset = research_hashing(nmsbits)
        lut0_size = (1 << nmsbits) * (1 if len(hashset) < 256 else 2)
        lut1_size = (1 << (16 - nmsbits)) * (len(hashset))
        total = lut0_size + lut1_size

        hashsets += [(nmsbits,
            {
                'lut0_size':lut0_size,
                'lut1_size':lut1_size,
                'total_size':total,
                'hashset':hashset
            }
        )]

    for nmsbits, result in hashsets:
        print '%d msbits: hash entries %d, lut0 %d bytes, lut1 %d bytes, total %d bytes' % (nmsbits,
                                                                                        len(result['hashset']),
                                                                                        result['lut0_size'],
                                                                                        result['lut1_size'],
                                                                                        result['total_size'])

    best_hashset = min(hashsets, key=lambda x: x[1]['total_size'])
    print ''
    print 'best hashset so far: %d msbits' % best_hashset[0]
    for s in best_hashset[1]['hashset']:
        print '\t ' + s
    return best_hashset[0]

def gen_luts(oplist, nmsbits):
    hashset = []

    top_lut = {}
    bot_lut = {}

    top_template = '{0:0%db}%s' % (nmsbits, ' ' * (16 - nmsbits))

    for i in range(1 << 16):
        fullmask = '{0:016b}'.format(i)
        topmask = top_template.format(i >> (16 - nmsbits))

        top_matches = []
        full_matches = []
        for idx, op in enumerate(oplist):
            if is_op_match(op['opcode'], topmask):
                top_matches += [str(idx + 1)]
            if is_op_match(op['opcode'], fullmask):
                full_matches += [str(idx + 1)]
        if not top_matches:
            top_matches += ['0']
        if not full_matches:
            full_matches += ['0']
        top_matches = ' '.join(top_matches)
        match = full_matches[0]
        try:
            hash_idx = hashset.index(top_matches)
        except:
            hashset += [top_matches]
            hash_idx = len(hashset) - 1

        bot_idx = i % (1 << (16 - nmsbits));
        top_idx = i / (1 << (16 - nmsbits));

        top_lut[top_idx] = hash_idx
        bot_lut.setdefault(hash_idx, {})
        bot_lut[hash_idx].setdefault(bot_idx, match)
    return top_lut, bot_lut

def main():
    oplist = create_oplist(SPEC)
    try:
        f = open('luts.pickle', 'r')
        nmsbits, lut0, lut1 = pickle.load(f)
        print 'Reusing existing luts'
    except:
        print 'Recalculating hashset ...'
        nmsbits = find_best_nmsbits(oplist)
        print 'Recalculating luts ...'
        lut0, lut1 = gen_luts(oplist, nmsbits)
        f = open('luts.pickle', 'w')
        pickle.dump((nmsbits, lut0, lut1), f)
        print 'Done'

    c_template_fname = 'avr_op_decoder.jinja'
    h_template_fname = 'avr_op_decoder_header.jinja'

    c_out_fname = 'avr_op_decoder.c'
    h_out_fname = 'avr_op_decoder.h'

    env = jinja2.Environment(trim_blocks=True, lstrip_blocks=True, undefined=jinja2.StrictUndefined, line_comment_prefix='##')
    c_template = env.from_string(open(c_template_fname, 'r').read())
    h_template = env.from_string(open(h_template_fname, 'r').read())

    # render and output
    out = c_template.render({
        'oplist':oplist,
        'operand_map':OPERAND_MAP,
        'lut0':lut0,
        'lut1':lut1,
        'nmsbits': nmsbits,
    })

    open(c_out_fname, 'w').write(out)

    out = h_template.render({
        'oplist':oplist,
        })

    open(h_out_fname, 'w').write(out)

main()
