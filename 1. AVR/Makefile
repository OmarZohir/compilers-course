PART:=atmega328p
CC:=clang --target=avr
CFLAGS:=-Wall -Os -mmcu=${PART} -I/usr/lib/avr/include/ -I/home/pcp17/material/include/ -DPART=\"${PART}\"
LDFLAGS:=-mmcu=${PART}
ASFLAGS:=-mmcu=${PART}
PROGRAM:=led

	.PHONY: all run clean
all: ${PROGRAM}.elf

${PROGRAM}.s: ${PROGRAM}.c
    $(CC) $(CFLAGS) -o $@ -S $^

${PROGRAM}.o: ${PROGRAM}.s
    avr-as ${ASFLAGS} -o $@ $^

${PROGRAM}.elf: ${PROGRAM}.o
    avr-gcc -o $@ $^ ${LDFLAGS}

run: ${PROGRAM}.elf
    simulator_cli $^

clean:
    rm -f ${PROGRAM}.elf ${PROGRAM}.o gtkwave_output.vcd ${PROGRAM}.s