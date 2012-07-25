#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

unsigned bit_position;
unsigned char *buf;

unsigned int get_bits(int k)
{
	unsigned int ret = 0;
	int count = 0;
	while (k > 0)
	{
		unsigned int bit;
		bit = ((buf[bit_position / 8] & (1 << ((bit_position%8))))>>((bit_position%8)));
		ret |= bit<<(count);
		count++;
		bit_position+=1;
		k--;
	}
	return ret;
}

unsigned int get_byte()
{
	return get_bits(8);
}

uint32_t get_uint32()
{
	return get_byte() | (get_byte()<<8) | (get_byte()<<16) | (get_byte()<<24);
}

void read_deflate()
{
	bool bfinal = get_bits(1);
	uint8_t btype = get_bits(2);
	printf("%x %x\n", bfinal, btype);
}

int main(int argc, char **argv)
{
	FILE *f = fopen(argv[1], "r");
	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);
	buf = (unsigned char*)malloc(size);
	fread(buf, 1,size, f);
	printf("%x %x\n", get_byte(), get_byte());
	int cm = get_byte();
	uint8_t flg = get_byte();
	uint32_t mtime = get_uint32();
	bool fname = flg & (1<<3);
	bool fextra = flg & (1<<2);
	uint8_t xfl = get_byte();
	uint8_t os = get_byte();
	printf("%x, %x, %x\n",flg,  xfl, os);
	if (fextra) {
		printf("extra\n");
	}
	if (fname) {
		uint8_t c;
		do {
			c = get_byte();
		} while (c);
	}
	read_deflate();
	uint32_t crc32 = get_uint32();
	uint32_t isize = get_uint32();
}
