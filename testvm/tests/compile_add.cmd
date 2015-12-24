avr-gcc -c -x assembler-with-cpp -mmcu=atmega88 -o add.o add.s
avr-ld -mavr4 -e init -o add.elf add.o
avr-objcopy -O binary add.elf add.bin
rm -f add.elf add.o
