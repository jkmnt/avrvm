avr-gcc -c -x assembler-with-cpp -mmcu=atmega88 -o muls.o muls.s
avr-ld -mavr4 -e init -o muls.elf muls.o
avr-objcopy -O binary muls.elf muls.bin
rm -f muls.elf muls.o
