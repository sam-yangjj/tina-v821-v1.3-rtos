#include <stdarg.h>
#include <stdbool.h>
#include <getopt.h>
#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include "stdint.h"
#include "rv_head.h"
#include "rv_elf.h"
#include "md5.h"
#include "crc32.h"


#define a_argument required_argument
static const char usage_short_opts[] = "S:a:@";
static struct option const usage_long_opts[] = {
	{"src-bin",	a_argument, NULL, 'S'},
	{"algorithm",a_argument, NULL, 'a'},
	{"symbols",	no_argument, NULL, '@'},
	{NULL,		no_argument, NULL, 0x0},
};
#define util_getopt_long() getopt_long(argc, argv, usage_short_opts, \
				       usage_long_opts, NULL)

typedef enum{
	ALGORITHM_MD5 = 0,
	ALGORITHM_CRC32,
}algorithm_t;

int main(int argc, char *argv[])
{
	FILE* fd;
	uint32_t img_len = 0;
	const char *src_path = NULL;
	algorithm_t algorithm = ALGORITHM_MD5;
	int opt;
	int i = 0;
	unsigned long change_addr = 0;
	unsigned char *strtab = 0;
	int ret = 0;

	while ((opt = util_getopt_long()) != EOF) {
		switch (opt) {
		case 'S':
			src_path = optarg;
			printf("src rv bin path = %s\n", src_path);
			break;
		case 'a':
			if(strcmp(optarg, "md5") == 0){
				algorithm = ALGORITHM_MD5;
				printf("algorithm = md5 \n");
			}else if(strcmp(optarg, "crc32") == 0){
				algorithm = ALGORITHM_CRC32;
				printf("algorithm = crc32 \n");
			}else{
				printf("algorithm %s not support!, will use md5 \n", optarg);
				algorithm = ALGORITHM_MD5;
			}
			break;
		default:
			printf("argv err \n");
			goto err0;
		}
	}

	if(src_path == NULL){
		printf("get src rv bin path failed! \n");
		goto err0;
	}

	/* open rv bin */
	fd = fopen(src_path, "rb+");
	if(fd == NULL){
		printf("file %s open failed! \n", src_path);
		goto err1;
	}
	/* get rv bin length*/
	fseek(fd, 0L, SEEK_END);
	img_len = ftell(fd);
	printf("rv.bin length = %d \n", img_len);

	/* read rv bin */
	char *pImg = NULL;
	pImg = (char *)malloc(img_len);

	if(pImg == NULL){
		printf("img malloc failed! \n");
		goto err2;
	}

	fseek(fd, 0L, SEEK_SET);
	fread(pImg, img_len, 1, fd);

	Elf32_Ehdr *ehdr = NULL; /* Elf header structure pointer */
	Elf32_Phdr *phdr = NULL; /* Program header structure pointer */
	Elf32_Shdr *shdr = NULL;
	ehdr = (Elf32_Ehdr *)pImg;
	phdr = (Elf32_Phdr *)(pImg + ehdr->e_phoff);
	shdr = (Elf32_Shdr *)(pImg + ehdr->e_shoff + (ehdr->e_shstrndx * sizeof(Elf32_Shdr)));

	unsigned long digest_addr = 0, digest_offset = 0;

	if (shdr->sh_type == SHT_STRTAB)
		strtab = (unsigned char *)(pImg + shdr->sh_offset);

	for (i = 0; i < ehdr->e_shnum; ++i) {
		shdr = (Elf32_Shdr *)(pImg + ehdr->e_shoff +
				     (i * sizeof(Elf32_Shdr)));

		if (!(shdr->sh_flags & SHF_ALLOC) ||
		    shdr->sh_addr == 0 || shdr->sh_size == 0) {
			continue;
		}

		if(strtab){
			if(strcmp((char*)&strtab[shdr->sh_name],".digest") == 0) {
				digest_addr = shdr->sh_addr;
				digest_offset = shdr->sh_offset;
			}
		}
	}

	if (digest_addr != 0) {
		if(algorithm == ALGORITHM_MD5){
			md5_state_t state;
			md5_byte_t  md5_digest[16];
			int i;

			memset(&state, 0, sizeof(state));
			md5_init(&state);
			md5_append(&state, (md5_byte_t *)pImg, img_len);
			md5_finish(&state, md5_digest);

			printf("[md5]Loading .digest @ 0x%08lx : ", digest_addr);
			for (i = 0; i < 16; i++)
				printf("%02x", md5_digest[i]);
			printf("\n");
			memcpy(pImg + digest_offset, &img_len, 4);
			memcpy(pImg + digest_offset + 4, md5_digest, 16);

		} else if(algorithm == ALGORITHM_CRC32){
			uint32_t cal_crc32 = 0;
			cal_crc32 = crc32(0, (unsigned char *)pImg, img_len);
			printf("[crc32]Loading .digest @ 0x%08lx : 0x%08x\n", digest_addr, cal_crc32);
			memcpy(pImg + digest_offset, &img_len, 4);
			memcpy(pImg + digest_offset + 4, &cal_crc32, 4);
		} else {
			printf("algorithm %d not support! \n", algorithm);
		}
	}

	/* write data to rv bin */
	fseek(fd, 0L, SEEK_SET);
	fwrite((void*)pImg, img_len, 1, fd);
	printf("add message to rv bin success ..\n");

err2:
	free(pImg);
err1:
	fclose(fd);
err0:
	return 0;
}
