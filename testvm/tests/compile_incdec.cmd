avr-gcc -c -x assembler-with-cpp -mmcu=atmega88 -o incdec.o incdec.s
avr-ld -mavr4 -e init -o incdec.elf incdec.o
avr-objcopy -O binary incdec.elf incdec.bin
rm -f incdec.elf incdec.o
