#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

FILE *df;

unsigned bit_position;
unsigned char *buf;
char obuf[1000000];
int buf_index = 0;

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
		//printf("length: %d next_code: %x\n", bits, code);
	}

	int max_code = code_length_count-1;
	for (int n=0; n <= max_code; n++) {
		int len = tree[n].length;
		if (len != 0) {
			tree[n].code = next_code[len];
			next_code[len]++;
		}
		//printf("%d %d %x\n", n, tree[n].length, tree[n].code);
	}

}

int read_huff(struct huff_node tree[], int N)
{
	//XXX: fix this
	int last_min = 0;
	int candidate = 0;
	while (1) {
		int min_length = 10000000;
		for (int i=0; i<N; i++) {
			if (tree[i].length > last_min && tree[i].length < min_length) {
				min_length = tree[i].length;
			}
		}
		//printf("min_length: %d\n", min_length);
		for (int i=0; i<(min_length - last_min); i++) {
			candidate <<= 1;
			candidate |= get_bits(1);
		}
		//printf("candidate: %x\n", candidate);
		for (int i=0; i<N; i++) {
			if (candidate == tree[i].code && min_length == tree[i].length) {
				//printf("code_val: %x\n", candidate);
				return i;
			}
		}
		last_min = min_length;
	}
}
void write_byte(uint8_t a)
{
	fwrite(&a, 1, 1, df);
}

void write_uint32(uint32_t a)
{
	fwrite(&a, 4, 1, df);
}




// the maximum length out of this is 15. The maximum huffcode is 18 We'll need to encode each of these as a byte
// instead of a nibble (plus an additional byte for the extra bits)
void read_length_codes(struct huff_node code_length_codes[], struct huff_node length_codes[], int count)
{
	for (int i=0; i<count; i++) {
		int code = read_huff(code_length_codes, 19);
		write_byte(code);
		if (code == 17) {
			int length = get_bits(3) + 3;
			write_byte(length-3);
			//printf("0 repeat: %d\n", length);
			for (int j=0; j<length; j++) {
				length_codes[i+j].length = 0;
				length_codes[i+j].code = 0;
			}
			i+=length-1;
		} else if (code == 18) {
			int length = get_bits(7) + 11;
			write_byte(length-11);
			//printf("0 repeat: %d\n", length);
			for (int j=0; j<length; j++) {
				length_codes[i+j].length = 0;
				length_codes[i+j].code = 0;
			}
			i+=length-1;
		} else if (code == 16) {
			int length = get_bits(2) + 3;
			write_byte(length-3);
			//printf("%d repeat: %d\n", length_codes[i-1].length, length);
			for (int j=0; j<length; j++) {
				length_codes[i+j].length = length_codes[i-1].length;
				length_codes[i+j].code = 0;
			}
			i+=length-1;
		} else {
			length_codes[i].length = code;
			length_codes[i].code = 0;
			//printf("lit: %d[%c] %d\n", i, i, code);
		}
	}

}
void read_dynamic_huffman()
{
	int literal_length_code_count = get_bits(5) + 257;
	write_byte(literal_length_code_count - 257);

	int distance_code_count = get_bits(5) + 1;
	write_byte(distance_code_count - 1);

	int code_length_code_count = get_bits(4) + 4;
	write_byte(code_length_code_count - 4);

	struct huff_node code_length_codes[19] = {};
	int code_length_code_order[19] = {
		16, 17, 18,
		0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
	//printf("dh %d %d %d\n", literal_length_code_count, distance_code_count, code_length_code_count);
	for (int i=0; i<code_length_code_count; i++) {
		code_length_codes[code_length_code_order[i]].length = get_bits(3);
		write_byte(code_length_codes[code_length_code_order[i]].length);
		//printf("%d -> %d\n", code_length_code_order[i], code_length_codes[code_length_code_order[i]].length);
	}

	// we need to output the huffman tables in the compact form because they are not canonical
	build_huff(code_length_codes, 19, 7);

	//printf("position %d\n", bit_position);
	struct huff_node literal_length_codes[literal_length_code_count];
	read_length_codes(code_length_codes, literal_length_codes, literal_length_code_count);
	//XXX: get real max
	build_huff(literal_length_codes, literal_length_code_count, 55);

	struct huff_node distance_codes[distance_code_count];
	read_length_codes(code_length_codes, distance_codes, distance_code_count);
	build_huff(distance_codes, distance_code_count, 55);

	while (1) {
	int code = read_huff(literal_length_codes, literal_length_code_count);
	{
		int extra_length_bits[] = {
			0, //257
			0, //258
			0, //259
			0, //260
			0, //261
			0, //262
			0, //263
			0, //264
			1, //265
			1, //266
			1, //267
			1, //268
			2, //269
			2, //270
			2, //271
			2, //272
			3, //273
			3, //274
			3, //275
			3, //276
			4, //277
			4, //278
			4, //279
			4, //280
			5, //281
			5, //282
			5, //283
			5, //284
			0, //285
		};
		int extra_distance_bits[] = {
			0,
				0,
				0,
				0,
				1,
				1,
				2,
				2,
				3,
				3,
				4,
				4,
				5,
				5,
				6,
				6,
				7,
				7,
				8,
				8,
				9,
				9,
				10,
				10,
				11,
				11,
				12,
				12,
				13,
				13};
		if (code < 256) {
			printf("%c", code);
			char zero = 0;
			fwrite(&zero, 1,1,df);
			obuf[buf_index++] = code;
		}
		else if (code == 256) {
			unsigned short dout = 32769;
			fwrite(&dout, 1,2, df);
			break;
		} else {
			int lengths[] = {3,4,5,6,7,8,9,10,11,13,
				15,17,19,23,27,31,35,43,51,59,
				67,83,99,115,131,163,195,227,258};
			int distance[] = {1,2,3,4,5,7,9,13,17,25,
				33,49,65,97,129,193,257,385,513,
				769,1025,1537,2049,3073,4097,6145,
				8193,12289,16385,24577};
			int extra_length = get_bits(extra_length_bits[code-257]);
			int distance_code = read_huff(distance_codes, distance_code_count);
			int extra_distance = get_bits(extra_distance_bits[distance_code]);
			//printf("MMM: %d %d\n", code-257, extra_length_bits[code-257]);
			//printf("LLL: %d %d\n", distance_code, extra_distance_bits[distance_code]);
			//printf("length code: %d extra: %d:%d distance: %d extra:%d\n",
			//       code, extra_length,
			//       distance_code, extra_distance);
			//printf("\nmatch %d %d\n", lengths[code-257] + extra_length,
			//      distance[distance_code] + extra_distance);
			int l = lengths[code-257] + extra_length;
			int d = distance[distance_code] + extra_distance;
			for (int i=0; i<l; i++) {
				char c = obuf[buf_index-d];
				obuf[buf_index++] = c;
				printf("%c",c);
				fflush(stdout);
			}
			unsigned char lout = l - 3;
			unsigned short dout = d;
			fwrite(&dout, 1,2, df);
			fwrite(&lout, 1,1, df);
		}
	}
	}
	fclose(df);
	//struct huff_node example_lengths[] = {{3},{3},{3},{3},{3},{2},{4},{4}};
	//build_huff(example_lengths, 8, 4);
}
void read_deflate()
{
	bool bfinal;
	do {
	bfinal = get_bits(1);
	uint8_t btype = get_bits(2);
	//printf("%x %x\n", bfinal, btype);
	write_byte(bfinal);
	write_byte(btype);
	if (btype == 2)
		read_dynamic_huffman();
	else
		abort();
	} while (!bfinal);
}

int main(int argc, char **argv)
{
	FILE *f = fopen(argv[1], "r");
	df = fopen("decisions", "w+");
	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);
	buf = (unsigned char*)malloc(size);
	fread(buf, 1,size, f);
	write_byte(get_byte());
	write_byte(get_byte());
	int cm = get_byte();
	write_byte(cm);
	uint8_t flg = get_byte();
	write_byte(flg);
	uint32_t mtime = get_uint32();

	bool fname = flg & (1<<3);
	bool fextra = flg & (1<<2);
	uint8_t xfl = get_byte();
	write_byte(xfl);
	uint8_t os = get_byte();
	write_byte(os);
	//printf("%x, %x, %x\n",flg,  xfl, os);
	if (fextra) {
		printf("extra\n");
	}
	if (fname) {
		uint8_t c;
		do {
			c = get_byte();
			write_byte(c);
		} while (c);
	}
	read_deflate();
	uint32_t crc32 = get_uint32();
	write_uint32(crc32);
	uint32_t isize = get_uint32();
	write_uint32(isize);
}
