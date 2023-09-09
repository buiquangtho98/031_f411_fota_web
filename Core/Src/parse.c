/*
 * parse.c
 *
 *  Created on: Aug 31, 2023
 *      Author: Tho
 */

#include <stdint.h>
#include <string.h>
#include "parse.h"

void version_parse(uint8_t *dst, uint8_t *src) {
	uint16_t i = 0;
	while (src[i] != '\r') {
		dst[i] = src[i];
		i++;
	}
}

int so_ky_tu(char *str) {
	int k = str[0], i = 0;
	while (k != 0) {
		k = str[++i];
	}
	return i;
}

char* subStr_pos(char *str, char *subStr, int size) {
	char *pc = 0;
	int i = 0;
	for (i = 0; i < (size - so_ky_tu(subStr) + 1); i++) {
		int k = 0;
		for (int j = 0; j < so_ky_tu(subStr); j++) {
			if (str[i + j] != subStr[j]) {
				k++;
			}
		}
		if (k == 0) {
			pc = &str[i];
			break;
		}
	}
	return pc;
}

void firmware_parse(char *dst, char *src, int size) {
	uint16_t j = 0;
	char *str_pos = 0;
	str_pos = src;
	int cur_size = size;
	while (subStr_pos(str_pos, "\r\n\r\n+IPD,", cur_size)) {
		uint16_t valid = subStr_pos(str_pos, "\r\n\r\n+IPD,", cur_size)
				- str_pos;
		for (int i = 0; i < valid; i++) {
			dst[j] = str_pos[i];
			j++;
		}
		str_pos += (valid + 9);
		cur_size -= (valid + 9);
		char* valid2 = subStr_pos(str_pos, ":",cur_size);
		cur_size -= (int)(valid2 + 1 - str_pos);
		str_pos = (valid2 + 1);
	}
	uint16_t valid = subStr_pos(str_pos, "\r\nCLOSED\r\n",cur_size) - str_pos;
	for (int i = 0; i < valid; i++) {
		dst[j] = str_pos[i];
		j++;
	}
}
