.text

.global init

.org 0
init:
         eor r1, r1
         rcall test_com
         rcall test_neg
         rcall test_inc
         rcall test_dec
         break                     // done

test_com:
         mov r21, r1               // clear a
com_a_loop:
         out 63, r1
         mov r24, r21
         com r24                   // printf will show result here
         subi r21, -1
         brne com_a_loop           // terminate at y = 256
         ret

test_neg:
         mov r21, r1               // clear a
neg_a_loop:
         out 63, r1
         mov r24, r21
         neg r24                   // printf will show result here
         subi r21, -1
         brne neg_a_loop           // terminate at y = 256
         ret

test_inc:
         mov r21, r1               // clear a
inc_a_loop:
         out 63, r1
         mov r24, r21
         inc r24                   // printf will show result here
         subi r21, -1
         brne inc_a_loop           // terminate at y = 256
         ret

test_dec:
         mov r21, r1               // clear a
dec_a_loop:
         out 63, r1
         mov r24, r21
         dec r24                   // printf will show result here
         subi r21, -1
         brne dec_a_loop           // terminate at y = 256
         ret
