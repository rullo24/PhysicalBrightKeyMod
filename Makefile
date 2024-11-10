# Compiler and flags
CC = gcc
CFLAGS = -Wall -Werror
LIBS = -luser32 -ldxva2 -lkernel32

# Source and output files
SRC = phys.c 
OUT = phys_brightkeymod  # The name of the output executable

# Default target to compile the program
all: $(OUT)
$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LIBS) $(TARGET)

# Clean target to remove the compiled output
clean:
	del $(OUT)