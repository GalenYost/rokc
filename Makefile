MAKEFLAGS += --no-print-directory

NOB_EXEC := nob

all: build

bootstrap:
	cc nob.c -o $(NOB_EXEC)

build: bootstrap
	./$(NOB_EXEC) build

run: bootstrap
	./$(NOB_EXEC) run

clear: bootstrap
	./$(NOB_EXEC) clear
