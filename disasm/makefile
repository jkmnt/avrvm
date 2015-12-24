HOST_SRC :=
HOST_SRC += disasm.c
HOST_SRC += ../avr_op_decoder.c

HOST_TGT := disasm

all: $(HOST_TGT).exe

$(HOST_TGT).exe: $(HOST_SRC)
	gcc -O2 -I. -I../ $^ -o $@

clean:
	rm -f $(HOST_TGT).exe
