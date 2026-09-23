# Makefile for Phantom Arcade MiSTer Framebuffer Frontend
# Run directly on MiSTer DE10-Nano or cross-compile with ARM toolchain

CC ?= gcc
CFLAGS ?= -O2 -Wall
TARGET = phantom_mister_frontend

all: $(TARGET)

$(TARGET): phantom_mister_frontend.c
	$(CC) $(CFLAGS) -o $(TARGET) phantom_mister_frontend.c

install: $(TARGET)
	cp $(TARGET) /media/fat/Scripts/
	chmod +x /media/fat/Scripts/$(TARGET)

clean:
	rm -f $(TARGET)
