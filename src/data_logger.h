/*
 * data_logger.h
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
#include "properties.h"

#ifndef HEADER_LOGGER
#define HEADER_LOGGER
void store_phrase(char *orig, char *trans, char *body, int lang_src, int lang_dst);
void load_settings_log (char *size, char *filename);
void save_phrases_to_file (void);
int clean_xml_file (char *filename);
int convert_to_anki (char *file_in, char *file_out, favorites * favorite_log);
#endif