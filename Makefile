SRC = $(wildcard src/*.c)
INPUT ?= $(wildcard input/inputs/*/*/*/*.x)
RM = rm -rf
RUN = run.py

ifeq ($(OS), Windows_NT)
	_OS = NT
	RM = erase /Q
else
	_OS = Linux
endif

.PHONY: all verify clean

all: main

main: $(SRC)
	gcc -g $^ -o $@

run: clean main
	@./script/$(RUN) 

clean:
	@-$(RM) main.exe 

dclean:
	@-$(RM) main.exe result