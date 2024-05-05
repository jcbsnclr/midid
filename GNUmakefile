# midid - software MIDI synthesiser, utilising JACK
# Copyright (C) 2024  Jacob Sinclair <jcbsnclr@outlook.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

CC?=gcc
DBG?=gdb

CSRC:=$(wildcard source/*.c)
COBJ:=$(patsubst source/%.c, build/%.c.o, $(CSRC))

CFLAGS+=-Og -g -Wall -Wextra  -Werror -Isource/ -c -MMD -fsanitize=undefined -fstack-protector-strong
LFLAGS+=-lm -fsanitize=undefined -fstack-protector-strong

DEPS:=jack

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
	$(DBG) -x util/gdb.txt --args $(BIN) -E "donk: 0.05s1.0 -> 0.2s0.5 -> SUST -> 0.6s0.0"   -O "o: wave=sin vol=1.0"   -I "foo donk: o * o"   -I "bar donk: o * o"

clean: 
	rm -rf build/

-include build/*.c.d
