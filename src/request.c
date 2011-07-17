/*
 * request.c
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

#include "request.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <regex.h>
#include <ctype.h>

#include <json-glib/json-glib.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <string.h>

struct string {
	char *ptr;
	size_t len;
};


void init_string(struct string *s) {
	s->len = 0;
	s->ptr = malloc(s->len+1);
	if (s->ptr == NULL) {
		fprintf(stderr, "malloc() failed\n");
		exit(EXIT_FAILURE);
	}
	s->ptr[0] = '\0';
}


size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s)
{
	size_t new_len = s->len + size*nmemb;
	s->ptr = realloc(s->ptr, new_len+1);
	if (s->ptr == NULL) {
		fprintf(stderr, "realloc() failed\n");
		exit(EXIT_FAILURE);
	}
	memcpy(s->ptr+s->len, ptr, size*nmemb);
	s->ptr[new_len] = '\0';
	s->len = new_len;

	return size*nmemb;
}


char *replace_str (char *str, char *orig, char *rep)
{
	static char buffer[4096];
	char *p;

	if(!(p = strstr(str, orig)))
	{
		return str;
	}

	strncpy(buffer, str, p-str);
	buffer[p-str] = '\0';

	sprintf(buffer+(p-str), "%s%s", rep, p+strlen(orig));

	return buffer;
}

char *clean_output (char *string, char *pattern, size_t len)
{
	int i;
	char *p;
	char string_new[len+1];
	string_new[0] = '\0';
	char *temp = string;
	size_t length_p = strlen(pattern);
	size_t j;
	while (p = strstr(temp, pattern))
	{
		j = p-temp;
		strncat (string_new, temp, j);
		temp = p+length_p;
	}
	strcat (string_new, temp);
	//g_print ("\n%s", string_new);
	return string_new;
}

int count_occurence (char *str, char *orig)
{
	int cnt=0;
	int orig_len = strlen(orig);
	char *buffer;
	buffer = strstr(str, orig);
	while(buffer)
	{
		buffer = strstr(buffer+orig_len, orig);
		cnt++;
	}
	return cnt;
}


void to_upper_case(char *q)
{
	unsigned char c;
	while (*q) { c = *q; *q = toupper(c); q++; }
}


char *getTranslation(char *text_to_trans, char *lang_src_str, char *lang_dst_str)
{
	CURL *curl_handle;
	gchar *encoded;
	gchar *utf8_trans;
	gchar full_text_to_trans[4096];
	struct string body;
	gchar *site_url = "http://translate.google.com/translate_a/t";
	
    init_string(&body);
	curl_handle = curl_easy_init();
	
	encoded = curl_easy_escape(curl_handle, text_to_trans, strlen(text_to_trans));

	snprintf (full_text_to_trans, 4096, "client=json&sl=%s&tl=%s&hl=%s&text=%s", lang_src_str, lang_dst_str,lang_dst_str, encoded);
	curl_free(encoded);
	//g_print("%s?%s", site_url, full_text_to_trans);

	struct curl_slist *headers=NULL;
  	headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
	headers = curl_slist_append(headers, "Host: www.google.com");
	headers = curl_slist_append(headers, "User-Agent: Mozilla/5.0");
	headers = curl_slist_append(headers, "Accept-Encoding: deflate");
	headers = curl_slist_append(headers, "Connection: close");
	
	curl_easy_setopt(curl_handle, CURLOPT_URL, site_url);
	curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(curl_handle, CURLOPT_POST, 1);
	curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, full_text_to_trans);
	curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);

	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, writefunc);
	curl_easy_setopt(curl_handle,   CURLOPT_WRITEHEADER, &body);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &body);

	curl_easy_perform(curl_handle);

	//cleanup curl stuff
	curl_easy_cleanup(curl_handle);
	//g_print("\n%s \nSize = %d", body.ptr, body.len);

	if(body.len)
	{
		//get charset from header
		gchar *char1 = strstr(body.ptr, "charset");
		gchar *char2 = strstr(char1+8, "\r\n");
		size_t length_ch = char2-(char1+8);
		char charset[length_ch+1];
		strncpy(charset, char1+8, length_ch);
		charset[length_ch] = '\0';
		//g_print("\n%s", charset);

		//get translation from body
		char *trans = strstr(body.ptr, "{");
		//g_print("\n%s", trans);

		if(strcmp(charset, "UTF-8")!=0)
		{
			GError *error = NULL;
			utf8_trans = strdup(g_convert (trans, -1, "UTF-8", charset, NULL, NULL, &error));
		}
		else
		{
			utf8_trans = strdup(trans);
		}
	}
	else
	{
		char *trans = "Internet connection problems\0";
		utf8_trans = strdup(trans);
	}
	free (body.ptr);
	
	
	return utf8_trans;
}


//dumb SJP parser
char *getSJP(char *text_to_trans)
{
	CURL *curl_handle;
	gchar *encoded, *utf8_trans;
	gchar full_text_to_trans[1024];
	struct string body;
	
    init_string(&body);
	curl_handle = curl_easy_init();
	
	encoded = curl_easy_escape(curl_handle, text_to_trans, strlen(text_to_trans));
	snprintf (full_text_to_trans, 1024, "http://sjp.pwn.pl/szukaj/%s", encoded);
	curl_free(encoded);
	//g_print("%s", full_text_to_trans);

	struct curl_slist *headers=NULL;
  	headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
	headers = curl_slist_append(headers, "Host: sjp.pwn.pl");
	headers = curl_slist_append(headers, "User-Agent: Mozilla/5.0");
	headers = curl_slist_append(headers, "Accept-Encoding: gzip,deflate");
	headers = curl_slist_append(headers, "Connection: keep-alive");
	
	curl_easy_setopt(curl_handle, CURLOPT_URL, full_text_to_trans);
	curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(curl_handle, CURLOPT_POST, 1);
	curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);

	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, writefunc);
	curl_easy_setopt(curl_handle,   CURLOPT_WRITEHEADER, &body);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &body);

	curl_easy_perform(curl_handle);

	// cleanup curl stuff
	curl_easy_cleanup(curl_handle);

	if(body.len)
	{
		size_t pos;
		char *pch1, *pch2, *pch3;
		size_t pos1, pos2, pos3;
		char trans_buffer[body.len];
		char *trans1 = strstr(body.ptr, "<!-- listahasel -->");
		if(trans1 != NULL)
		{
			char *trans2 = strstr(trans1+20, "<!-- /listahasel -->");
			pos = trans2-trans1;
			char trans3[pos+pos/4];
			strncpy (trans3, trans1+20, pos);
			trans3[pos] = '\0';
			strcpy(trans3, g_convert (trans3, -1, "UTF-8", "ISO-8859-2", NULL, NULL, NULL));
			trans_buffer[0] = '\0';

			int new_line_cond = 0;
			int semicolon_cond = 0;
			char *pch = strtok (trans3,";");
			while (pch != NULL)
			{
				if(strstr(pch, "<a href") != NULL)
				{
					if(new_line_cond)
					{
						strcat (trans_buffer, "\n");
					}
					pch1 = strstr(pch, "a href");
					pch2 = strchr(pch1, '>');
					pch3 = strchr(pch1, '<');
					pos1 = pch2-pch1+1;
					pos2 = pch3-pch1-pos1;
					//g_print ("\n%d %d", pos1, pos2);
					if(pos2 != -1)
					{
						strncat (trans_buffer, pch1+pos1, pos2);
					}
					strcat (trans_buffer, "\n");
					//g_print("\n%s\n", trans_buffer);
					new_line_cond = 1;
					semicolon_cond = 1;
				}
				if(strstr(pch, "&#187") != NULL)
				{
					pos3 = strlen(pch)-5;
					strcat (trans_buffer, "- ");
					strncat (trans_buffer, pch, pos3);
					strcat (trans_buffer, "\n");
					//g_print("\n%s\n", trans_buffer);
				}
				if(strstr(pch, "&#8226") != NULL)
				{
					semicolon_cond = 0;
				}
				if(semicolon_cond && strstr(pch, "&#187") == NULL && 
				   strstr(pch, "&#171") == NULL && 
				   strstr(pch, "/listahasel") == NULL &&
				   strstr(pch, "<a href") == NULL)
				{
					strcat (trans_buffer, "- ");
					strcat (trans_buffer, pch);
					strcat (trans_buffer, "\n");
					//g_print("\n%s\n", trans_buffer);
				}
				pch = strtok (NULL, ";");
			}
		}
		else
		{
			trans_buffer[0] = '\0';
			strcat (trans_buffer, "Nie znaleziono haseł spełniających podany warunek\0");
		}
		//g_print("\n%s", trans_buffer);
		utf8_trans = strdup (trans_buffer);
	}
	else
	{
		char *trans = "Internet connection problems\0";
		utf8_trans = strdup(trans);
	}
	free (body.ptr);
	
	return utf8_trans;
}


char *getOneLook(char *text_to_trans)
{
	int i;
	CURL *curl_handle;
	gchar *encoded, *utf8_trans;
	gchar full_text_to_trans[1024];
	struct string body;
	
    init_string(&body);
	curl_handle = curl_easy_init();
	
	encoded = curl_easy_escape(curl_handle, text_to_trans, strlen(text_to_trans));

	snprintf (full_text_to_trans, 1024, "http://www.onelook.com/?xml=1&w=%s", encoded);
	curl_free(encoded);
	//g_print("%s", full_text_to_trans);

	struct curl_slist *headers=NULL;
  	headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
	headers = curl_slist_append(headers, "Host: www.onelook.com");
	headers = curl_slist_append(headers, "User-Agent: Mozilla/5.0");
	headers = curl_slist_append(headers, "Accept-Encoding: deflate");
	headers = curl_slist_append(headers, "Connection: keep-alive");
	headers = curl_slist_append(headers, "Keep-alive: 115");
	
	curl_easy_setopt(curl_handle, CURLOPT_URL, full_text_to_trans);
	curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(curl_handle, CURLOPT_POST, 1);
	curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);

	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, writefunc);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &body);

	curl_easy_perform(curl_handle);

	//cleanup curl stuff
	curl_easy_cleanup(curl_handle);

	if(body.len)
	{
		//g_print("\n%s\n%d", body.ptr, body.len);
		char temp_trans[body.len];
		char trans_buffer[body.len];

		if (body.len>600 && strstr(body.ptr, "<OLQuickDef>") != NULL)
		{
			strcpy (temp_trans, "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n<OLResponse>\n");
			strcat (temp_trans, strstr(body.ptr, "<OLQuickDef>"));	
			
			xmlInitParser();
			strcpy (trans_buffer, text_to_trans);
			strcat (trans_buffer, execute_xpath_expression (temp_trans, "//OLQuickDef", NULL, body.len));
			strcat (trans_buffer, "\nPhrases:");
			strcat (trans_buffer, execute_xpath_expression (temp_trans, "//OLPhrases", NULL, body.len));
			strcat (trans_buffer, "\nSimilar:");
			strcat (trans_buffer, execute_xpath_expression (temp_trans, "//OLSimilar", NULL, body.len));
			strcat (trans_buffer, "\0");
			if(strstr(&trans_buffer, "&lt;i&gt;"))
			{
				strcpy(trans_buffer, clean_output (&trans_buffer, "&lt;i&gt;", strlen(trans_buffer)));
				strcpy(trans_buffer, clean_output (&trans_buffer, "&lt;/i&gt;", strlen(trans_buffer)));
			}
			xmlCleanupParser();
			//g_print (trans_buffer);
		}	
		else
		{
			trans_buffer[0] = '\0';
			strcpy(trans_buffer, "Not found\0");
		}
		utf8_trans = strdup (trans_buffer);
	}
	else
	{
		char *trans = "Internet connection problems\0";
		utf8_trans = strdup(trans);
	}
	free (body.ptr);

	return utf8_trans;
}


char *parse_translation(gchar *json_out){
	JsonParser *parser;
	JsonNode *root;
	GError *error;
	gchar simple_trans[strlen(json_out)];
	gchar *result;
	int i, j;

	parser = json_parser_new ();
	error = NULL;
	//g_print("\n%s", json_out);
	json_parser_load_from_data (parser, json_out, -1, &error);
	if (error)
	{
		//g_print ("Unable to parse %s: %s\n",json_out , error->message);
		g_error_free (error);
		g_object_unref (parser);
		result = strdup(json_out);
		return result;
	}

	JsonReader *reader = json_reader_new (json_parser_get_root (parser));

	json_reader_read_member (reader, "sentences"); //1

	if (json_reader_count_elements (reader)<1)
	{
		g_object_unref (parser);
		gchar out[] = "Not found\0";
		result = strdup(out);
		return result;	
	}

	if(json_reader_count_elements (reader)>1)
	{
		simple_trans[0] = '\0';
		for(j=0; j<json_reader_count_elements (reader); j++)
		{
			json_reader_read_element (reader, j); //2
			json_reader_read_member (reader, "trans"); //4
			strcat (simple_trans, json_reader_get_string_value (reader));
			if(j==0)
			{
				strcat (simple_trans, "\n ");
			}
			json_reader_end_member (reader); //4
			json_reader_end_element (reader); //2
		}
		json_reader_end_member (reader); //1
	}
	else
	{
		json_reader_read_element (reader, 0); //2
		json_reader_read_member (reader, "orig"); //3
		strcpy (simple_trans, json_reader_get_string_value (reader));
		strcat (simple_trans, " - ");
		json_reader_end_member (reader); //3
		json_reader_read_member (reader, "trans"); //4
		//char *to_upper = json_reader_get_string_value (reader);
		//to_upper_case (to_upper);
		strcat (simple_trans, json_reader_get_string_value (reader));
		json_reader_end_member (reader); //4
		json_reader_end_element (reader); //2
		json_reader_end_member (reader); //1
	}

	json_reader_read_member (reader, "dict"); //1

	for(j=0; j<json_reader_count_elements (reader); j++)
	{
		json_reader_read_element (reader, j); //2
		json_reader_read_member (reader, "pos"); //3
		strcat(simple_trans, "\n\n");
		strcat(simple_trans, json_reader_get_string_value (reader));
		strcat(simple_trans, "\n");
		json_reader_end_member (reader); //3
		json_reader_read_member (reader, "terms"); //4

		for(i=0; i<json_reader_count_elements (reader); i++){
			if(i!=0)
			{
				strcat(simple_trans, ", ");
			}
			json_reader_read_element (reader, i); //5
			strcat(simple_trans, json_reader_get_string_value (reader));
			json_reader_end_element (reader); //5

		}
		json_reader_end_member (reader); //4
		json_reader_end_element (reader); //2
	}
	json_reader_end_member (reader); //1
	//g_print("\n%s", simple_trans);
	
	g_object_unref (parser);

	result = strdup (simple_trans);

	return result;
}