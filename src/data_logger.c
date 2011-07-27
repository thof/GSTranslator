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
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

phrase* phrases=NULL;
int size_p, current_index;

void save_phrases_to_file (char *filename);


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
		xmlInitParser();
		char filename[128];
		sprintf(filename, "%s/words.xml", getenv("HOME"));
		save_phrases_to_file (filename);
		xmlCleanupParser();
		current_index=0;
	}
	phrases[current_index].src = lang_src;
	phrases[current_index].dst = lang_dst;
	strcpy(phrases[current_index].orig, orig);
	strcpy(phrases[current_index].trans, trans);
	if(body==NULL)
	{
		strcpy(phrases[current_index].body, body);
	}
	else
	{
		strcpy(phrases[current_index].body, body+2);
	}
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


void save_phrases_to_file (char *filename)
{
	xmlDocPtr doc;
	xmlNodePtr parent, child;
	xmlXPathContextPtr xpathCtx;
	xmlXPathObjectPtr xpathObj;
	int i, cond=0;
	char temp[16];
	
	xmlKeepBlanksDefault(0);
	doc = xmlParseFile(filename);
	if (doc == NULL)
	{
		doc = xmlNewDoc (BAD_CAST "1.0");
		parent = xmlNewNode(NULL, "phrases");
		xmlDocSetRootElement(doc, parent);
	}
	else
	{
		cond = 1;
		xpathCtx = xmlXPathNewContext(doc);
		if(xpathCtx == NULL)
		{
			fprintf(stderr,"Error: unable to create new XPath context\n");
			xmlFreeDoc(doc);
			return;
		}

		// add keyboard shortcuts
		xpathObj = xmlXPathEvalExpression("/phrases", xpathCtx);
		if(xpathObj == NULL) 
		{
			fprintf(stderr,"Error: unable to evaluate xpath expression\n");
			xmlXPathFreeContext(xpathCtx);
			xmlFreeDoc(doc); 
			return;
		}
		parent = xpathObj->nodesetval->nodeTab[0];
	}
	for(i=0; i<size_p; i++)
	{
		child = xmlNewChild (parent, NULL, "phrase", NULL);

		sprintf(temp, "%d",phrases[i].src);
		xmlNewChild (child, NULL, "src", temp);
		sprintf(temp, "%d", phrases[i].dst);
		xmlNewChild (child, NULL, "dst", temp);
		xmlNewChild (child, NULL, "orig", phrases[i].orig);
		xmlNewChild (child, NULL, "trans", phrases[i].trans);
		xmlNewChild (child, NULL, "body", phrases[i].body);
	}

	if(cond)
	{
		xmlXPathFreeObject(xpathObj);                                  
		xmlXPathFreeContext(xpathCtx);
	}
	xmlSaveFormatFile (filename, doc, 1);
	xmlFreeDoc(doc);
}