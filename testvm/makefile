HOST_SRC :=
HOST_SRC += testvm.c
HOST_SRC += ../avr_op_decoder.c
HOST_SRC += ../avrvm.c

HOST_TGT := testvm

all: $(HOST_TGT).exe

$(HOST_TGT).exe: $(HOST_SRC)
	gcc -std=c99 -O2 -DDEBUG_AVRVM=1 -I. -I../ $^ -o $@

clean:
	rm -f $(HOST_TGT).exe
