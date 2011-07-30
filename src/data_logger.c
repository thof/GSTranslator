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
#include "request.h"

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
void convert_to_anki (char *file_in, char *file_out);


char *replace_char (char *string, char pattern, char *replace, size_t len)
{
	int i;
	char *p;
	char string_new[2*len];
	string_new[0] = '\0';
	char *temp = string;
	size_t j;
	while (p = strchr(temp, pattern))
	{
		j = p-temp;
		strncat (string_new, temp, j);
		strcat (string_new, replace);
		temp = p+1;
		g_print ("\n%s\n",string_new);
	}
	strcat (string_new, temp);
	//g_print ("\n%s", string_new);
	return string_new;
}


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
	//clean_xml_file ("/home/rad/words.xml");
	convert_to_anki ("/home/rad/words.xml", "/home/rad/words.txt");
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


void convert_to_anki (char *file_in, char *file_out)
{
	xmlDocPtr doc;
	xmlNodePtr node_p, child, child_p, next_p, temp;
	xmlXPathContextPtr xpathCtx;
	xmlXPathObjectPtr xpathObj;
	char body_temp[4096];
	FILE *file = NULL, *file_temp = NULL;
  	file = fopen(file_out, "w");
	file_temp = fopen(file_in, "r");
	int size;


	if (file==NULL || file_temp==NULL) 
	{
  		g_print("Error in opening a file.");
		return;
  	}

	fseek(file_temp, 0, SEEK_END);
	size = ftell(file_temp);
	g_print ("\n%d", size);
	fclose(file_temp);
	char phrases_out[size];
	
	xmlKeepBlanksDefault(0);
	doc = xmlParseFile(file_in);
	if (doc == NULL) {
		fprintf(stderr, "Error: unable to parse file \"%s\"\n", file_in);
		return;
	}

	xpathCtx = xmlXPathNewContext(doc);
	if(xpathCtx == NULL) {
		fprintf(stderr,"Error: unable to create new XPath context\n");
		xmlFreeDoc(doc); 
		return;
	}

	xpathObj = xmlXPathEvalExpression("/phrases/phrase", xpathCtx);
	if(xpathObj == NULL) {
		fprintf(stderr,"Error: unable to evaluate xpath expression\n");
		xmlXPathFreeContext(xpathCtx);
		xmlFreeDoc(doc); 
		return;
	}
	node_p = xpathObj->nodesetval->nodeTab[0];

	sprintf (phrases_out, "#This file was generated by GSTranslator");
	do
	{
		child_p = node_p->children->next->next;
		strcpy (body_temp, xmlNodeGetContent (child_p->next->next));
		strcpy (body_temp, replace_char (&body_temp, '\n', "</br>", strlen(body_temp)));
		sprintf (phrases_out + strlen (phrases_out), "\n%s\t<b>%s</b></br></br><font size=\"2\">%s</font>",
		         xmlNodeGetContent (child_p), xmlNodeGetContent (child_p->next),
		         body_temp);
		g_print(phrases_out);
	}
	while(node_p=(node_p->next));

	fwrite(phrases_out, strlen(phrases_out), 1, file);
	fclose(file);
	
	xmlXPathFreeObject(xpathObj);                                  
	xmlXPathFreeContext(xpathCtx);
	xmlFreeDoc(doc);
}


void clean_xml_file (char *filename)
{
	xmlDocPtr doc;
	xmlNodePtr node_p, child, child_p, next_p, temp;
	xmlXPathContextPtr xpathCtx;
	xmlXPathObjectPtr xpathObj;
	int size, i, cond=0;
	
	xmlKeepBlanksDefault(0);
	doc = xmlParseFile(filename);
	if (doc == NULL) {
		fprintf(stderr, "Error: unable to parse file \"%s\"\n", filename);
		return;
	}

	xpathCtx = xmlXPathNewContext(doc);
	if(xpathCtx == NULL) {
		fprintf(stderr,"Error: unable to create new XPath context\n");
		xmlFreeDoc(doc); 
		return;
	}

	xpathObj = xmlXPathEvalExpression("/phrases/phrase", xpathCtx);
	if(xpathObj == NULL) {
		fprintf(stderr,"Error: unable to evaluate xpath expression\n");
		xmlXPathFreeContext(xpathCtx);
		xmlFreeDoc(doc); 
		return;
	}

	node_p = xpathObj->nodesetval->nodeTab[0];

	do
	{
		child_p = node_p->children;
		g_print ("\n%s %s %s %s", xmlNodeGetContent (child_p), 
			         xmlNodeGetContent (child_p->next), 
			         xmlNodeGetContent (child_p->next->next),
			         xmlNodeGetContent (child_p->next->next->next)
			         );
		next_p = node_p->next;
		do
		{
			cond=0;
			if(!next_p)
			{
				break;
			}
			child=(next_p->children);
			g_print ("\n***%s %s %s %s", xmlNodeGetContent (child), 
			         xmlNodeGetContent (child->next), 
			         xmlNodeGetContent (child->next->next),
			         xmlNodeGetContent (child->next->next->next)
			         );
			if(strcmp(xmlNodeGetContent (child), xmlNodeGetContent (child_p))==0 &&
			   strcmp(xmlNodeGetContent (child->next), xmlNodeGetContent (child_p->next))==0 &&
			   strcmp(xmlNodeGetContent (child->next->next), xmlNodeGetContent (child_p->next->next))==0
			   )
			{
				temp = next_p->prev;
				xmlUnlinkNode (next_p);
				next_p = temp;
				//xmlFreeNode (next_p);
				//cond=1;
			}
		}
		while((next_p=(next_p->next)));
	}
	while(node_p=(node_p->next));

	xmlXPathFreeObject(xpathObj);                                  
	xmlXPathFreeContext(xpathCtx);
	xmlSaveFormatFile (filename, doc, 1);
	xmlFreeDoc(doc);
}

