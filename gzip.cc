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

struct huff_node
{
	int length;
	int code;
};

void build_huff(struct huff_node tree[], int code_length_count, int max_length)
{
	int bl_count[max_length+1];
	int next_code[max_length+1];
	for (int i=0; i<max_length; i++) {
		bl_count[i] = 0;
	}
	for (int i=0; i<code_length_count; i++) {
		bl_count[tree[i].length]++;
	}
	int code = 0;
	bl_count[0] = 0;
	for (int bits = 1; bits <= max_length; bits++) {
		code = (code + bl_count[bits-1]) << 1;
		next_code[bits] = code;
	}

	int max_code = code_length_count-1;
	for (int n=0; n <= max_code; n++) {
		int len = tree[n].length;
		if (len != 0) {
			tree[n].code = next_code[len];
			next_code[len]++;
		}
	}

}

void read_dynamic_huffman()
{
	int literal_length_code_count = get_bits(5) + 257;
	int distance_code_count = get_bits(5) + 1;
	int code_length_code_count = get_bits(4) + 4;
	int code_length_codes[19] = {};
	int code_length_code_order[19] = {
		16, 17, 18,
		0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
	printf("dh %d %d %d\n", literal_length_code_count, distance_code_count, code_length_code_count);
	for (int i=0; i<code_length_code_count; i++) {
		code_length_codes[code_length_code_order[i]] = get_bits(3);
		printf("%d -> %d\n", code_length_code_order[i], code_length_codes[code_length_code_order[i]]);
	}

	struct huff_node example_lengths[] = {{3},{3},{3},{3},{3},{2},{4},{4}};
	build_huff(example_lengths, 8, 4);
}
void read_deflate()
{
	bool bfinal = get_bits(1);
	uint8_t btype = get_bits(2);
	printf("%x %x\n", bfinal, btype);
	if (btype == 2)
		read_dynamic_huffman();
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
