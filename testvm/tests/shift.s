.text

.global init

.org 0
init:
         eor r1, r1
         mov r3, r1
         inc r3
         rcall test_lsr
         rcall test_asr
         rcall test_ror
         break                     // done

test_lsr:
         mov r21, r1               // clear a
lsr_a_loop:
         out 63, r1
         mov r24, r21
         lsr r24                   // printf will show result here
         subi r21, -1
         brne lsr_a_loop           // terminate at y = 256
         ret

test_asr:
         mov r21, r1               // clear a
asr_a_loop:
         out 63, r1
         mov r24, r21
         asr r24                   // printf will show result here
         subi r21, -1
         brne asr_a_loop           // terminate at y = 256
         ret

test_ror:
         mov r21, r1               // clear a
ror_a_loop:
         out 63, r1
         mov r24, r21
         ror r24                   // printf will show result here
         out 63, r3
         mov r24, r21
         ror r24                   // printf will show result here
         subi r21, -1
         brne ror_a_loop           // terminate at y = 256
         ret
