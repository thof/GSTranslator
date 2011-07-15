/*
 * xml_parser.c
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
#include "xml_parser.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

char *execute_xpath_expression (const char* filename, const xmlChar* xpathExpr,
                                const xmlChar* nsList, int size);
char *print_xpath_nodes (xmlNodeSetPtr nodes);
int *get_xpath_nodes_size (const char* filename, const xmlChar* xpathExpr,
                           const xmlChar* nsList);
int *size_xpath_nodes (xmlNodeSetPtr nodes);

void save_config_file (const char* filename, shortcuts * shortcut, 
                       favorites * favorite, hidden_dicts * h_dict)
{
	xmlDocPtr doc;
	xmlNodePtr parent;
	xmlXPathContextPtr xpathCtx;
	xmlXPathObjectPtr xpathObj;
	int size, i;
	char temp_val[16];
	
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

	// add keyboard shortcuts
	xpathObj = xmlXPathEvalExpression("//normal_notify_hotkey", xpathCtx);
	if(xpathObj == NULL) {
		fprintf(stderr,"Error: unable to evaluate xpath expression\n");
		xmlXPathFreeContext(xpathCtx);
		xmlFreeDoc(doc); 
		return;
	}
	xmlNodeSetContent (xpathObj->nodesetval->nodeTab[0], shortcut->name);
	xmlXPathFreeObject(xpathObj);

	xpathObj = xmlXPathEvalExpression("//wide_notify_hotkey", xpathCtx);
	if(xpathObj == NULL) {
		fprintf(stderr,"Error: unable to evaluate xpath expression\n");
		xmlXPathFreeContext(xpathCtx);
		xmlFreeDoc(doc); 
		return;
	}
	shortcut++;
	xmlNodeSetContent (xpathObj->nodesetval->nodeTab[0], shortcut->name);
	xmlXPathFreeObject(xpathObj);

	xpathObj = xmlXPathEvalExpression("//favorite_hotkey", xpathCtx);
	if(xpathObj == NULL) {
		fprintf(stderr,"Error: unable to evaluate xpath expression\n");
		xmlXPathFreeContext(xpathCtx);
		xmlFreeDoc(doc); 
		return;
	}
	shortcut++;
	xmlNodeSetContent (xpathObj->nodesetval->nodeTab[0], shortcut->name);
	xmlXPathFreeObject(xpathObj);

	xpathObj = xmlXPathEvalExpression("//favorite_hotkey_back", xpathCtx);
	if(xpathObj == NULL) {
		fprintf(stderr,"Error: unable to evaluate xpath expression\n");
		xmlXPathFreeContext(xpathCtx);
		xmlFreeDoc(doc); 
		return;
	}
	shortcut++;
	xmlNodeSetContent (xpathObj->nodesetval->nodeTab[0], shortcut->name);
	xmlXPathFreeObject(xpathObj);

	// delete all favorites
	xpathObj = xmlXPathEvalExpression("//favorite", xpathCtx);
	if(xpathObj == NULL) {
		fprintf(stderr,"Error: unable to evaluate xpath expression\n");
		xmlXPathFreeContext(xpathCtx);
		xmlFreeDoc(doc); 
		return;
	}
	size = (xpathObj->nodesetval) ? xpathObj->nodesetval->nodeNr : 0;
	for(i = 0; i < size; ++i) {
		if(xpathObj->nodesetval->nodeTab[i]->type == XML_ELEMENT_NODE) {
			xmlUnlinkNode(xpathObj->nodesetval->nodeTab[i]);
		}
	}
	xmlXPathFreeObject(xpathObj);

	//delete all hidden elements
	xpathObj = xmlXPathEvalExpression("//hidden", xpathCtx);
	if(xpathObj == NULL) {
		fprintf(stderr,"Error: unable to evaluate xpath expression\n");
		xmlXPathFreeContext(xpathCtx);
		xmlFreeDoc(doc); 
		return;
	}
	size = (xpathObj->nodesetval) ? xpathObj->nodesetval->nodeNr : 0;
	for(i = 0; i < size; ++i) {
		if(xpathObj->nodesetval->nodeTab[i]->type == XML_ELEMENT_NODE) {
			xmlUnlinkNode(xpathObj->nodesetval->nodeTab[i]);
		}
	}
	xmlXPathFreeObject(xpathObj);

	xpathObj = xmlXPathEvalExpression("/config", xpathCtx);
	if(xpathObj == NULL) {
		fprintf(stderr,"Error: unable to evaluate xpath expression\n");
		xmlXPathFreeContext(xpathCtx);
		xmlFreeDoc(doc); 
		return;
	}
	// add favorites
	do
	{
		parent = xmlNewChild (xpathObj->nodesetval->nodeTab[0], NULL, "favorite", NULL);
		sprintf(temp_val, "%d", favorite->src_code);
		//g_print("\nval = %s", temp_val);
		xmlNewChild (parent, NULL, "src_lang", temp_val);
		sprintf(temp_val, "%d", favorite->dst_code);
		//g_print("\nval = %s", temp_val);
		xmlNewChild (parent, NULL, "dst_lang", temp_val);
		favorite++;
	}
	while(favorite->src_code != NULL);
	// add hidden elements
	do
	{
		sprintf(temp_val, "%d", h_dict->code);
		parent = xmlNewChild (xpathObj->nodesetval->nodeTab[0], NULL, "hidden", temp_val);
		h_dict++;
	}
	while(h_dict->code != NULL);                                  
	xmlXPathFreeObject(xpathObj);

	xmlXPathFreeContext(xpathCtx);
	xmlSaveFormatFile (filename, doc, 1);
	//xmlDocDump(stdout, doc);
	xmlFreeDoc(doc);
}


void save_size_position (const char* filename, int *w_width, int *w_height, 
                         int *w_x, int *w_y)
{
	xmlDocPtr doc;
	xmlXPathContextPtr xpathCtx;
	xmlXPathObjectPtr xpathObj;
	char temp_val[16];

	//g_print ("\nSave %d %d %d %d", *w_width, *w_height, *w_x, *w_y);
	
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

	xpathObj = xmlXPathEvalExpression("//default_width", xpathCtx);
	if(xpathObj == NULL) {
		fprintf(stderr,"Error: unable to evaluate xpath expression\n");
		xmlXPathFreeContext(xpathCtx);
		xmlFreeDoc(doc); 
		return;
	}
	sprintf(temp_val, "%d", *w_width);
	xmlNodeSetContent (xpathObj->nodesetval->nodeTab[0], temp_val);
	xmlXPathFreeObject(xpathObj);

	xpathObj = xmlXPathEvalExpression("//default_height", xpathCtx);
	if(xpathObj == NULL) {
		fprintf(stderr,"Error: unable to evaluate xpath expression\n");
		xmlXPathFreeContext(xpathCtx);
		xmlFreeDoc(doc); 
		return;
	}
	sprintf(temp_val, "%d", *w_height);
	xmlNodeSetContent (xpathObj->nodesetval->nodeTab[0], temp_val);
	xmlXPathFreeObject(xpathObj);

	xpathObj = xmlXPathEvalExpression("//default_x", xpathCtx);
	if(xpathObj == NULL) {
		fprintf(stderr,"Error: unable to evaluate xpath expression\n");
		xmlXPathFreeContext(xpathCtx);
		xmlFreeDoc(doc); 
		return;
	}
	sprintf(temp_val, "%d", *w_x);
	xmlNodeSetContent (xpathObj->nodesetval->nodeTab[0], temp_val);
	xmlXPathFreeObject(xpathObj);

	xpathObj = xmlXPathEvalExpression("//default_y", xpathCtx);
	if(xpathObj == NULL) {
		fprintf(stderr,"Error: unable to evaluate xpath expression\n");
		xmlXPathFreeContext(xpathCtx);
		xmlFreeDoc(doc); 
		return;
	}
	sprintf(temp_val, "%d", *w_y);
	xmlNodeSetContent (xpathObj->nodesetval->nodeTab[0], temp_val);
	xmlXPathFreeObject(xpathObj);
	                                  
	xmlXPathFreeContext(xpathCtx);
	xmlSaveFormatFile (filename, doc, 1);
	//xmlDocDump(stdout, doc);
	xmlFreeDoc(doc);
}


char *execute_xpath_expression (const char* filename, const xmlChar* xpathExpr, const xmlChar* nsList, int size)
{
	xmlDocPtr doc;
	xmlXPathContextPtr xpathCtx;
	xmlXPathObjectPtr xpathObj;

	/* Load XML document */
	if(size != 0)
	{
		doc = xmlParseMemory(filename, size);
	}
	else
	{
		doc = xmlParseFile(filename);
	}
	if (doc == NULL) {
		fprintf(stderr, "Error: unable to parse file \"%s\"\n", filename);
		return;
	}

	/* Create xpath evaluation context */
	xpathCtx = xmlXPathNewContext(doc);
	if(xpathCtx == NULL) {
		fprintf(stderr,"Error: unable to create new XPath context\n");
		xmlFreeDoc(doc); 
		return;
	}

	/* Evaluate xpath expression */
	xpathObj = xmlXPathEvalExpression(xpathExpr, xpathCtx);
	if(xpathObj == NULL) {
		fprintf(stderr,"Error: unable to evaluate xpath expression \"%s\"\n", xpathExpr);
		xmlXPathFreeContext(xpathCtx);
		xmlFreeDoc(doc); 
		return;
	}

	char *content = strdup(print_xpath_nodes(xpathObj->nodesetval));

	/* Cleanup */
	xmlXPathFreeObject(xpathObj);
	xmlXPathFreeContext(xpathCtx); 
	xmlFreeDoc(doc); 

	//g_print("\n%s\n%d", content, strlen(content));
	return content;
}


char *print_xpath_nodes(xmlNodeSetPtr nodes)
{
	xmlNodePtr cur;
	int size;
	int i;
	char result[4096];
	strcpy(result, "");

	size = (nodes) ? nodes->nodeNr : 0;

	//fprintf(output, "Result (%d nodes):\n", size);
	for(i = 0; i < size; ++i) {

		if(nodes->nodeTab[i]->type == XML_ELEMENT_NODE) {
			cur = nodes->nodeTab[i];
			//fprintf(output, "= element node \"%s\" content %s\n", cur->name, contt);
			strcat(result, xmlNodeGetContent (cur));
		}
	}
	//g_print("\n%s", result);
	return result;
}


int *get_xpath_nodes_size(const char* filename, const xmlChar* xpathExpr, const xmlChar* nsList) 
{
	xmlDocPtr doc;
	xmlXPathContextPtr xpathCtx;
	xmlXPathObjectPtr xpathObj;

	/* Load XML document */
	doc = xmlParseFile(filename);
	if (doc == NULL) {
		fprintf(stderr, "Error: unable to parse file \"%s\"\n", filename);
		return;
	}

	/* Create xpath evaluation context */
	xpathCtx = xmlXPathNewContext(doc);
	if(xpathCtx == NULL) {
		fprintf(stderr,"Error: unable to create new XPath context\n");
		xmlFreeDoc(doc); 
		return;
	}

	/* Evaluate xpath expression */
	xpathObj = xmlXPathEvalExpression(xpathExpr, xpathCtx);
	if(xpathObj == NULL) {
		fprintf(stderr,"Error: unable to evaluate xpath expression \"%s\"\n", xpathExpr);
		xmlXPathFreeContext(xpathCtx);
		xmlFreeDoc(doc); 
		return;
	}

	/* Print results */
	int size = size_xpath_nodes(xpathObj->nodesetval);
	//char *content = strdup(cont);

	/* Cleanup */
	xmlXPathFreeObject(xpathObj);
	xmlXPathFreeContext(xpathCtx); 
	xmlFreeDoc(doc); 

	return size;
}


int *size_xpath_nodes(xmlNodeSetPtr nodes) {
	int size;

	size = (nodes) ? nodes->nodeNr : 0;

	return size;
}