
.text

.global init

.org 0
init:
         eor r1, r1
         rcall test_add
         rcall test_adc
         rcall test_sub
         rcall test_sbc
         break                     // done


test_add:
         mov r21, r1               // clear a
add_a_loop:
         mov r20, r1               // clear b
add_b_loop:
         out 63, r1
         mov r24, r21
         mov r22, r20
         add r24, r22              // printf will show result here

         subi r20, -1
         brne add_b_loop           // terminate at x = 256
         subi r21, -1
         brne add_a_loop           // terminate at y = 256
         ret

test_adc:
         ldi r18, 1
         mov r21, r1               // clear a
adc_a_loop:
         mov r20, r1               // clear b
adc_b_loop:
         out 63, r1                // sreg = 0
         mov r24, r21
         mov r22, r20
         adc r24, r22              // printf will show result here

         out 63, r18               // sreg = 1
         mov r24, r21
         mov r22, r20
         adc r24, r22              // printf will show result here

         subi r20, -1
         brne adc_b_loop           // terminate at x = 256
         subi r21, -1
         brne adc_a_loop           // terminate at y = 256
         ret

test_sub:
         mov r21, r1               // clear a
sub_a_loop:
         mov r20, r1               // clear b
sub_b_loop:
         out 63, r1
         mov r24, r21
         mov r22, r20
         sub r24, r22              // printf will show result here

         subi r20, -1
         brne sub_b_loop           // terminate at x = 256
         subi r21, -1
         brne sub_a_loop           // terminate at y = 256
         ret

test_sbc:
         mov r21, r1               // clear a
sbc_a_loop:
         mov r20, r1               // clear b
sbc_b_loop:
         mov r18, r1               // clear s
sbc_s_loop:
         out 63, r18
         mov r24, r21
         mov r22, r20
         sbc r24, r22              // printf will show result here
         subi r18, -1
         cpi r18, 4
         brne sbc_s_loop           // terminate at s = 4
         subi r20, -1
         brne sbc_b_loop           // terminate at x = 256
         subi r21, -1
         brne sbc_a_loop           // terminate at y = 256
         ret
