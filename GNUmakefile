CC?=gcc
DBG?=gdb

CSRC:=$(wildcard source/*.c)
COBJ:=$(patsubst source/%.c, build/%.c.o, $(CSRC))

CFLAGS+=-O3 -g -Wall -Wextra  -Wno-missing-field-initializers -Isource/ -c -MMD
LFLAGS+=-lm

DEPS:=linenoise jack

CFLAGS+=$(foreach dep, $(DEPS), $(shell pkg-config --cflags $(dep)))
LFLAGS+=$(foreach dep, $(DEPS), $(shell pkg-config --libs $(dep)))

BIN:=build/midid

build/%.c.o: source/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@

$(BIN): $(COBJ)
	$(CC) $(LFLAGS) $(COBJ) -o $@

.PHONY: all run debug clean

all: $(BIN)

run: $(BIN)
	./$(BIN) $(RUNARGS) 

debug: $(BIN)
	$(DBG) $(BIN)

clean: 
	rm -rf build/

-include build/*.c.d
