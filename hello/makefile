HOST_SRC :=
HOST_SRC += host_hello.c
HOST_SRC += ../avr_op_decoder.c
HOST_SRC += ../avrvm.c

SCRIPT_SRC :=
SCRIPT_SRC += script_hello.c

HOST_TGT := host_hello
SCRIPT_TGT := script_hello

all: $(SCRIPT_TGT).lst run

run: $(HOST_TGT).exe $(SCRIPT_TGT).bin
	$(HOST_TGT).exe $(SCRIPT_TGT).bin

$(HOST_TGT).exe: $(HOST_SRC)
	gcc -O2 -std=c99 -Wall -I. -I../ $^ -o $@

$(SCRIPT_TGT).elf: $(SCRIPT_SRC)
	avr-gcc -Wall -Os -ffreestanding -mmcu=atmega88 -o $@ $^

$(SCRIPT_TGT).bin: $(SCRIPT_TGT).elf
	avr-objcopy -O binary $^ $@

$(SCRIPT_TGT).lst: $(SCRIPT_TGT).elf
	avr-objdump -D $^ > $@

clean:
	rm -f $(SCRIPT_TGT).bin $(SCRIPT_TGT).elf $(SCRIPT_TGT).lst $(HOST_TGT).exe
