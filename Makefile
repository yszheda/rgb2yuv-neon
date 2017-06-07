CC = arm-linux-androideabi-gcc

neon: neon.c
#	armcc -c --cpu=7 --debug neon.c -o neon.o
#	armlink --entry=EnableNEON neon.o -o neon.axf
#	arm-linux-gnueabi-gcc -flax-vector-conversions -mfloat-abi=softfp -mfpu=neon -o neon neon.c
#	arm-linux-gnueabi-gcc -v -I/usr/lib/gcc/arm-linux-androideabi/4.7.4/include-fixed/ -marm -march=armv7 -mfloat-abi=softfp -mfpu=neon -o neon neon.c
#	arm-linux-gnueabi-gcc -v -I/usr/local/ARMCompiler6.7/include -marm -march=armv7 -mfloat-abi=softfp -mfpu=neon -o neon neon.c
#	arm-linux-gnueabi-gcc -v -I/usr/local/DS-5_v5.27.0/sw/gcc/arm-linux-gnueabihf/ -lrt -marm -march=armv7 -mfloat-abi=softfp -mfpu=neon -o neon neon.c
#	arm-linux-gnueabi-gcc -v -lrt -mfloat-abi=softfp -mfpu=neon -o neon neon.c -static
#	arm-none-eabi-gcc -I/usr/local/DS-5_v5.27.0/sw/gcc/arm-linux-gnueabihf/ -lrt -marm -mfloat-abi=softfp -mfpu=neon -o neon neon.c
#	arm-none-eabi-gcc -v -marm -march=armv7 -mfloat-abi=softfp -mfpu=neon -o neon neon.c
#	arm-linux-gnueabihf-gcc -v -lrt -march=armv7 -mfpu=neon -o neon neon.c -static
#	/usr/local/DS-5_v5.27.0/sw/gcc/bin/arm-linux-gnueabihf-gcc -v -lrt -march=armv7 -mfpu=neon -o neon neon.c -static

#	armcc -O3 -Otime --vectorize --cpu=Cortex-A8 -o neon-func.o -c neon-func.c
#	arm-linux-gnueabi-gcc -O3 -mfloat-abi=softfp -mfpu=neon -o neon-func-gcc.o -c neon-func.c
#	arm-linux-androideabi-gcc -v -lrt -mfloat-abi=softfp -mfpu=neon -o neon neon.c -static
	$(CC) -v -mfloat-abi=softfp -mfpu=neon -o neon neon.c -static

serial: serial.c
	$(CC) -lrt -o serial serial.c

test: gen_testcase.c
	$(CC) -o gen_testcase gen_testcase.c
