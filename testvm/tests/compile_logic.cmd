avr-gcc -c -x assembler-with-cpp -mmcu=atmega88 -o logic.o logic.s
avr-ld -mavr4 -e init -o logic.elf logic.o
avr-objcopy -O binary logic.elf logic.bin
rm -f logic.elf logic.o
