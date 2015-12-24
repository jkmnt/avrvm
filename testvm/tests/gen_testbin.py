# python
# all possible avr opcodes, including undefined

import struct

out = ''
for i in range(65536):
    if i == 0b1001010111011000:    # elpm, skip
        continue
    if i & 0b1111111000001110 == 0b1001000000000110:    # elpm, skip
        continue
    if i & 0b1111111100001111 == 0b1001010000001011:    # des, skip
        continue
    if i == 0x95e8 or i == 0x95f8 :  # spm, skip
        continue
    if i == 0x95e8 or i == 0x9519 or i == 0x9419: # eicall, eijmp skip
        continue
    if i == 0x9598: # break
        continue

    out += struct.pack('<H', i)
    out += struct.pack('<H', i)

open('testbin.bin', 'wb').write(out)
