# You *MUST* use TI's branch of GCC 8.2.  GCC4 as distributed with Debian
# uses a different calling convention and is not compatible with the
# RF430FRL152H's ROM functions.

# So memory is *very* tight and the FRAM is really more of a patch
# against the main ROM, so rather than link together a complete
# executable with an entry point and RESET vector, we just link all
# the functions together with no start files and then produce our own
# patch table (applications) or entry point rigger (shellcode).

# As a rule, one .c file compiles to one .txt application.  We don't
# have room to abstract away complicated libraries.

# GCC 8.2

# Compiler and Flags
CC = msp430-elf-gcc

# Compiler Flags
CFLAGS = -mmcu=rf430frl152h -Wall \
         -Wno-unused-but-set-variable \
         -I. -I/home/runner/msp430-gcc/include \
         -Os

# Linker Flags
LDFLAGS = -Wl,--no-gc-sections -T rf430frl152h.ld

# Application files
APPS = main.txt

# Default target
all: $(APPS)

# Link object files to create ELF
%.elf: %.o
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# Convert ELF to HEX
%.hex: %.elf
	msp430-elf-objcopy $< -O ihex $@

# Convert HEX to TXT
%.txt: %.hex
	srec_cat $< -Intel -Output $@ -Texas_Instruments_TeXT

# Compile C files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean targets
.PHONY: clean
clean:
	rm -f *.elf *.hex *.txt *.o

