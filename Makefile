CC     = gcc
CFLAGS = -Wall -O3 -std=c99 $(FLAGS)
LFLAGS = -lm
TARGET = bin/simulator

all: $(TARGET)

$(TARGET): build/simulator.o build/parser.o build/random.o build/misc.o
	@echo "Building $@"
	$(CC) $(CFLAGS) $+ -o $@ $(LFLAGS)

build/%.o: src/%.c
	@echo "Compiling $<"
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(TARGET) build/*
