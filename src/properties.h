/*
 * properties.h
 *
 * Copyright (C) 2011 - 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HEADER_PROPERTIES
#define HEADER_PROPERTIES
typedef struct
{
	char name[32];
	char code[8];
	char flag[16];
	int successor;
	int predecessor;
} language;

typedef struct
{
	char name[512];
} shortcuts;

typedef struct
{
	int src_code;
	int dst_code;
} favorites;

typedef struct
{
	int code;
} hidden_dicts;

typedef struct
{
	int src;
	int dst;
	char orig[256];
	char trans[256];
	char body[1024];
} phrase;

int create_properties_window(char *conf_file, int deploy, language * dictionaries);
#endif