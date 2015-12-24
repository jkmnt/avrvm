
.text

.global init

.org 0
init:
         rcall test_eor
         break                     // done


test_eor:
         mov r21, r1               // clear a
eor_a_loop:
         mov r20, r1               // clear b
eor_b_loop:
         out 63, r1
         mov r24, r21
         mov r22, r20
         eor r24, r22              // printf will show result here

         subi r20, -1
         brne eor_b_loop           // terminate at x = 256
         subi r21, -1
         brne eor_a_loop           // terminate at y = 256
         ret
