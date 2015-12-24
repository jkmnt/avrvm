.text

.global init

.org 0
init:
         eor r1, r1
         eor r3, r3
         rcall test_mul
         rcall test_muls
         rcall test_mulsu
         rcall test_fmul
         rcall test_fmuls
         rcall test_fmulsu
         break                     // done


test_mul:
         push r0
         push r1
         ldi r21, 0               // clear a
mul_a_loop:
         ldi r20, 0               // clear b
mul_b_loop:
         out 63, r3
         mov r23, r21
         mov r22, r20
         mul r23, r22              // printf will show result here
         subi r20, -1
         brne mul_b_loop           // terminate at x = 256
         subi r21, -1
         brne mul_a_loop           // terminate at y = 256
         pop r1
         pop r0
         ret

test_muls:
         push r0
         push r1
         ldi r21, 0               // clear a
muls_a_loop:
         ldi r20, 0               // clear b
muls_b_loop:
         out 63, r3
         mov r23, r21
         mov r22, r20
         muls r23, r22              // printf will show result here
         subi r20, -1
         brne muls_b_loop           // terminate at x = 256
         subi r21, -1
         brne muls_a_loop           // terminate at y = 256
         pop r1
         pop r0
         ret

test_mulsu:
         push r0
         push r1
         ldi r21, 0               // clear a
mulsu_a_loop:
         ldi r20, 0               // clear b
mulsu_b_loop:
         out 63, r3
         mov r23, r21
         mov r22, r20
         mulsu r23, r22              // printf will show result here
         subi r20, -1
         brne mulsu_b_loop           // terminate at x = 256
         subi r21, -1
         brne mulsu_a_loop           // terminate at y = 256
         pop r1
         pop r0
         ret

test_fmul:
         push r0
         push r1
         ldi r21, 0               // clear a
fmul_a_loop:
         ldi r20, 0               // clear b
fmul_b_loop:
         out 63, r3
         mov r23, r21
         mov r22, r20
         fmul r23, r22              // printf will show result here
         subi r20, -1
         brne fmul_b_loop           // terminate at x = 256
         subi r21, -1
         brne fmul_a_loop           // terminate at y = 256
         pop r1
         pop r0
         ret

test_fmuls:
         push r0
         push r1
         ldi r21, 0               // clear a
fmuls_a_loop:
         ldi r20, 0               // clear b
fmuls_b_loop:
         out 63, r3
         mov r23, r21
         mov r22, r20
         fmuls r23, r22              // printf will show result here
         subi r20, -1
         brne fmuls_b_loop           // terminate at x = 256
         subi r21, -1
         brne fmuls_a_loop           // terminate at y = 256
         pop r1
         pop r0
         ret

test_fmulsu:
         push r0
         push r1
         ldi r21, 0               // clear a
fmulsu_a_loop:
         ldi r20, 0               // clear b
fmulsu_b_loop:
         out 63, r3
         mov r23, r21
         mov r22, r20
         fmulsu r23, r22              // printf will show result here
         subi r20, -1
         brne fmulsu_b_loop           // terminate at x = 256
         subi r21, -1
         brne fmulsu_a_loop           // terminate at y = 256
         pop r1
         pop r0
         ret

break
