CC = gcc

serial: serial.c
	$(CC) -o serial serial.c

test: gen_testcase.c
	$(CC) -o gen_testcase gen_testcase.c
