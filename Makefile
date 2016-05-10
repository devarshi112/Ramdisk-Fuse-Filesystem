ramdisk: ramdisk.c 
	gcc -g -Wall `pkg-config fuse --cflags --libs` -o ramdisk ramdisk.c

clean:
	rm ramdisk
