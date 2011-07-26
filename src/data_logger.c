/*
 * data_logger.c
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
#include "xml_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <gtk/gtk.h>


phrase* phrases=NULL;
int size_p, current_index;

void print_phrases()
{
	int i;	
	for(i=0; i<current_index; i++)
	{
		if(phrases[i].src==NULL)
		{
			return;
		}
		/*g_print ("\n***%d %d %s %s %s***\n", phrases[i].src, phrases[i].dst,
		         phrases[i].orig, phrases[i].trans, phrases[i].body);*/
		g_print ("\n***%s", phrases[i].orig);
	}
	g_print ("\n");
}

void store_phrase(char *orig, char *trans, char *body, int lang_src, int lang_dst)
{
	int i;
	for (i=0; i<current_index; i++)
	{
		if (strcmp(orig, phrases[i].orig)==0)
		{
			return;
		}
	}
	if(current_index>=size_p)
	{
		//save to file
		current_index=0;
	}
	phrases[current_index].src = lang_src;
	phrases[current_index].dst = lang_dst;
	strcpy(phrases[current_index].orig, orig);
	strcpy(phrases[current_index].trans, trans);
	strcpy(phrases[current_index].body, body);
	current_index++;
	print_phrases();
}


void size_phrases (int size)
{
	current_index = 0;
	if(phrases!=NULL)
	{
		free(phrases);
		phrases=NULL;
	}
	size_p = size;
	phrases = malloc (size*sizeof(phrase));
}