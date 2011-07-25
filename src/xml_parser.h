/*
 * xml_parser.h
 *
 * Copyright (C) thof 2011 <> 
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

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#ifndef HEADER_XML
#define HEADER_XML
void save_config_file (const char* filename, shortcuts * shortcut,
                       favorites * favorite, hidden_dicts * h_dict);
void save_languages_to_xml (language * languages, int size);
void save_size_position (const char* filename, int *w_width, int *w_height, 
                         int *w_x, int *w_y);
void add_favorite_node (const char* filename, const xmlChar* xpathExpr);
void load_languages_from_xml (char *filename, language * dictionaries, int sizeof_dicts);
char *execute_xpath_expression (const char* filename, const xmlChar* xpathExpr,
                                const xmlChar* nsList, int size);
char *print_xpath_nodes (xmlNodeSetPtr nodes);
int *get_xpath_nodes_size (const char* filename, const xmlChar* xpathExpr,
                           const xmlChar* nsList);
int *size_xpath_nodes (xmlNodeSetPtr nodes);
#endif