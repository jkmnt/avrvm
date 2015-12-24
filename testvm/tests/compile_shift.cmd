avr-gcc -c -x assembler-with-cpp -mmcu=atmega88 -o shift.o shift.s
avr-ld -mavr4 -e init -o shift.elf shift.o
avr-objcopy -O binary shift.elf shift.bin
rm -f shift.elf shift.o
