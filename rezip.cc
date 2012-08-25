#include "rezip.h"

int main(int argc, char **argv)
{
	FILE *f = fopen(argv[1], "r");
	df = fopen("result", "w+");
	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);
	buf = (unsigned char*)malloc(size);
	fread(buf, 1,size, f);
	put_byte(get_byte());
	put_byte(get_byte());
	int cm = get_byte();
	put_byte(cm);
	uint8_t flg = get_byte();
	put_byte(flg);
	uint32_t mtime = get_uint32();
	put_uint32(mtime);

	bool fname = flg & (1<<3);
	bool fextra = flg & (1<<2);
	uint8_t xfl = get_byte();
	put_byte(xfl);
	uint8_t os = get_byte();
	put_byte(os);
	//printf("%x, %x, %x\n",flg,  xfl, os);
	if (fextra) {
		printf("extra\n");
	}
	if (fname) {
		uint8_t c;
		do {
			c = get_byte();
			put_byte(c);
		} while (c);
	}
	write_deflate();
	uint32_t crc32 = get_uint32();
	//put_uint32(crc32);
	uint32_t isize = get_uint32();
	//put_uint32(isize);
	fwrite(outbuf, out_bit_position / 8, 1, df);
	fclose(df);
}
