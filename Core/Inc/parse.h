/*
 * parse.h
 *
 *  Created on: Aug 31, 2023
 *      Author: Tho
 */

#ifndef INC_PARSE_H_
#define INC_PARSE_H_

void version_parse();
char* subStr_pos(char *str, char *subStr, int size);
void firmware_parse(char *dst, char *src, int size);

#endif /* INC_PARSE_H_ */
