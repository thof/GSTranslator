/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * main.c
 * Copyright (C) thof 2011 <>
 * 
 * gstranslator is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * gstranslator is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "request.h"
#include "xml_parser.h"
#include "properties.h"
#include "data_logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <keybinder.h>
#include <libnotify/notify.h>
#include <glib.h>

typedef struct
{
	GtkWidget *window;
	GtkWidget *translate_button;
	GtkWidget *swap_button;
	GtkWidget *src;
	GtkWidget *dest;
	GtkWidget *clipboard;
	GtkWidget *status_icon;
	GtkWidget *menuitem_properties;
	GtkWidget *menuitem_about;
	GtkWidget *scan_label;
	GtkWidget *about_dialog;
	GtkWidget *gstrans_paned;
	GtkWidget *trans_spinner;
	gchar result[4096];
	gchar src_text[4096];
} Widgets;


GtkComboBoxText *cb_source_lang, *cb_dest_lang;
NotifyNotification *notify_open, *notify_test;
gchar *translated_text, *summary;
gchar *favorite_key, *favorite_key_backward, *normal_notify_key, *wide_notify_key;
gint lang_src, lang_dst, close_notify, favorite_index, favorite_size, deploy, temp;
gint *w_width, *w_height, *w_x, *w_y, paned_pos, trans_method;
gint fav_src[32], fav_dst[32];
gint sizeof_dicts = 60;
gchar recent_clip[4096], conf_file[512];
size_t src_length;
language dictionaries[60];
gint successor[60];
gboolean thread_cond, clipboard_cond = FALSE;

static gpointer thread_trans (gpointer user_data);
static gboolean print_result (gpointer user_data);
static void window_get_focus(GtkWidget *widget, GdkEvent *event, gpointer user_data);
static void normal_notify_handler(const char *keystring, gpointer user_data);
static void wide_notify_handler(const char *keystring, gpointer user_data);
static gboolean on_key_press(GtkWidget *widget, GdkEventKey *pKey, gpointer user_data);
static gboolean clean_src_textview(GtkWidget *widget, GdkEventKey* pKey, gpointer user_data);
static void translate (gpointer user_data);
static void translate_from_textview (GtkButton *button, gpointer user_data);
static void compare_textview_clipboard (GtkClipboard *clipboard, 
                                const char *received_text, gpointer user_data);
static int translate_normal_notify (GtkClipboard *clipboard, 
                             const char *received_text, gpointer user_data);
static int translate_wide_notify (GtkClipboard *clipboard, 
                           const char *received_text, gpointer user_data);
static void translate_from_clipboard (const char *received_text);
static void wide_notify_close (GtkWidget *widget, gpointer user_data);
static void change_src_lang (GtkWidget *widget, gpointer user_data);
static void change_dst_lang (GtkWidget *widget, gpointer user_data);
static void swap_lang (GtkWidget *widget, gpointer user_data);
static void init_config (gpointer user_data);
static void init_languages (gpointer user_data);
static void load_settings (gpointer user_data);
static void change_favorite (gpointer user_data);
static void change_favorite_back (gpointer user_data);
static void open_properties (GtkMenuItem *menuitem, gpointer user_data);
static int compare_ints (const void *a, const void *b);
static void destroy_window (GtkWidget *object, gpointer user_data); 
static void show_window (GtkStatusIcon *status_icon, GdkEvent *event, gpointer user_data);
static void get_size_position (GObject *gobject, GParamSpec *pspec, gpointer user_data);
static void get_focus_widget (GtkWidget *widget, GdkEvent *event, gpointer user_data);
static void open_about_dialog (GtkMenuItem *menuitem, gpointer user_data);


int main (int argc, char *argv[])
{
	GtkBuilder *builder;
	Widgets *widgets;
	GtkTextBuffer *buffer_src, *buffer_dest;
	GdkPixbuf *pixbuffer;
	GError *error=NULL;
	if(!g_thread_supported())
	{
		g_thread_init( NULL );
	}
    gdk_threads_init();
    gdk_threads_enter();
	
	deploy = 1;
	gtk_init (&argc, &argv);
	recent_clip[0] = '\0';
	close_notify = 1;

	// Construct a GtkBuilder instance and load UI description
	builder = gtk_builder_new ();
	if(deploy)
	{
		gtk_builder_add_from_file (builder, 
		                           PACKAGE_DATA_DIR"/gstranslator/ui/gstranslator.ui", 
		                           NULL);
		pixbuffer = gdk_pixbuf_new_from_file (PACKAGE_DATA_DIR"/icons/hicolor/64x64/apps/gstranslator.png",
		                                     &error);
		if(error!=NULL)
		{
			g_print ("Icon error -> %s", error->message);
		}
	}
	else
	{
		gtk_builder_add_from_file (builder, "src/gstranslator.ui", NULL);
		pixbuffer = gdk_pixbuf_new_from_file("src/icon/128x128/gstranslator.png", &error);
		if(error!=NULL)
		{
			g_print ("Icon error -> %s", error->message);
		}
	}

	widgets = g_slice_new (Widgets);
	
	widgets->window = gtk_builder_get_object (builder, "main_window");
	if(argc>1 && (strcmp(argv[1], "-d")==0 || strcmp(argv[1], "--daemon")==0))
	{
		gtk_widget_hide (widgets->window);
	}

	widgets->menuitem_properties = gtk_builder_get_object (builder, "menuitem_properties");
	widgets->menuitem_about = gtk_builder_get_object (builder, "menuitem_about");
	cb_source_lang = gtk_builder_get_object (builder, "sourcelang_button");
	cb_dest_lang = gtk_builder_get_object (builder, "destlang_button");
	widgets->swap_button = gtk_builder_get_object (builder, "swap_button");
	g_signal_connect (widgets->swap_button, "clicked", G_CALLBACK (swap_lang), widgets);
	widgets->translate_button = gtk_builder_get_object (builder, "translate_button");
	widgets->src = gtk_builder_get_object (builder, "source");
	buffer_src = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widgets->src));
	widgets->dest = gtk_builder_get_object (builder, "destination");
	buffer_dest = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widgets->dest));
	widgets->clipboard = gtk_clipboard_get (GDK_SELECTION_PRIMARY);
	widgets->scan_label = gtk_builder_get_object (builder, "scan_label");
	widgets->about_dialog = gtk_builder_get_object (builder, "about_dialog");
	widgets->gstrans_paned = gtk_builder_get_object (builder, "gstrans_paned");
	widgets->trans_spinner = gtk_builder_get_object (builder, "trans_spinner");
	
	notify_init("GSTranslator");
	notify_open = notify_notification_new ("", "", NULL);

	//widgets->status_icon = gtk_status_icon_new ();
	//g_signal_connect (widgets->status_icon, "button_press_event", G_CALLBACK (show_window), widgets);
	gtk_window_set_icon(widgets->window, pixbuffer);
	keybinder_init ();
	xmlInitParser ();
	init_config (widgets);
	init_languages (widgets);
	

	g_signal_connect (widgets->window, "destroy", G_CALLBACK (destroy_window), widgets);
	g_signal_connect (widgets->menuitem_properties, "activate", G_CALLBACK (open_properties), widgets);
	g_signal_connect (widgets->menuitem_about, "activate", G_CALLBACK (open_about_dialog), widgets);
	g_signal_connect (cb_source_lang, "changed", G_CALLBACK (change_src_lang), widgets);
	g_signal_connect (cb_dest_lang, "changed", G_CALLBACK (change_dst_lang), widgets);
	g_signal_connect (widgets->src, "key_press_event", G_CALLBACK (on_key_press), widgets);
	g_signal_connect (widgets->translate_button, "clicked", G_CALLBACK (translate_from_textview), widgets);
	g_signal_connect (widgets->src, "key_release_event", G_CALLBACK (clean_src_textview), widgets);
	
	g_signal_connect (widgets->window, "delete-event", G_CALLBACK (get_size_position), widgets);
	g_signal_connect (widgets->src, "focus-in-event", G_CALLBACK (window_get_focus), widgets);
	g_signal_connect (widgets->dest, "focus-in-event", G_CALLBACK (get_focus_widget), widgets);
	
	g_object_unref (builder);
	gtk_main ();
	gdk_threads_leave();
	
	g_slice_free (Widgets, widgets);

	return 0;
}

static gpointer thread_trans (gpointer user_data)
{
	gchar *result_trans, *json_out;
	Widgets *widgets = (Widgets*)user_data;
	
	//g_print ("\nText to trans: %s", widgets->src_text);
	switch (trans_method)
	{
		case 0:
			result_trans = getSJP(widgets->src_text); break;
		case 1:
			result_trans = getOneLook(widgets->src_text); break;
		case 2:
			json_out = getTranslation(widgets->src_text, 
			                          dictionaries[lang_src].code, 
			                          dictionaries[lang_dst].code);
			result_trans = parse_translation(json_out, lang_src, lang_dst);
			g_free (json_out);
			break;
	}
	strncpy(widgets->result, result_trans, 4096);
	g_free (result_trans);
	thread_cond = FALSE;
	gtk_spinner_stop (widgets->trans_spinner);
	return NULL;
}

gboolean print_result (gpointer user_data)
{
	if(!thread_cond)
	{
		GtkTextBuffer *buffer_dest, *buffer_src;
		Widgets *widgets = (Widgets*)user_data;
		//g_print ("\nWait for result");
		buffer_dest = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widgets->dest));
		gtk_text_buffer_set_text (buffer_dest, widgets->result, -1);
		if(clipboard_cond)
		{
			buffer_src = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widgets->src));
			gtk_text_buffer_set_text (buffer_src, widgets->src_text, -1);
			clipboard_cond = FALSE;
		}
		return FALSE;
	}
	return TRUE;
}


void window_get_focus (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	Widgets *widgets = (Widgets*)user_data;
	
	gtk_widget_set_sensitive (widgets->scan_label, TRUE);
	if(gtk_toggle_button_get_active (widgets->scan_label) && gtk_window_is_active  (widgets->window) 
	   && gtk_window_has_toplevel_focus (widgets->window))
	{
		gtk_clipboard_request_text (widgets->clipboard, compare_textview_clipboard, user_data);
	}
}


void normal_notify_handler (const char *keystring, gpointer user_data) 
{
	Widgets *widgets = (Widgets*)user_data;
	if(!close_notify)
	{
		notify_notification_close (notify_open, NULL);
	}
	close_notify = 1;
	notify_notification_set_urgency (notify_open, NOTIFY_URGENCY_NORMAL);
	gtk_clipboard_request_text (widgets->clipboard, translate_normal_notify, user_data);
}


void wide_notify_handler (const char *keystring, gpointer user_data) 
{
	Widgets *widgets = (Widgets*)user_data;
	notify_notification_set_urgency (notify_open, NOTIFY_URGENCY_CRITICAL);
	gtk_clipboard_request_text (widgets->clipboard, translate_wide_notify, user_data);
}


int translate_normal_notify (GtkClipboard *clipboard, 
                             const char *received_text, gpointer data)
{
	if(received_text==NULL || !strcmp(received_text, recent_clip))
	{
		//g_print("\nEven\n");
		notify_notification_show(notify_open, NULL);
	}
	else
	{
		//g_print("\n%s*", received_text);
		translate_from_clipboard (received_text);
		notify_notification_update (notify_open, summary, translated_text, NULL);
		notify_notification_show (notify_open, NULL);
		g_free (summary);
		g_free (translated_text);
	}
}


int translate_wide_notify (GtkClipboard *clipboard, 
                           const char *received_text, gpointer data)
{
	if(received_text==NULL || !strcmp(received_text, recent_clip))
	{
		if(close_notify)
		{
			notify_notification_show(notify_open, NULL);
			close_notify = 0;
		}
		else
		{
			notify_notification_close (notify_open, NULL);
			close_notify = 1;
		}
	}
	else
	{
		translate_from_clipboard (received_text);
		notify_notification_update (notify_open, summary, translated_text, NULL);
		notify_notification_show(notify_open, NULL);
		close_notify = 0;
		g_free (summary);
		g_free (translated_text);
	}
}


void translate (gpointer user_data)
{
	GThread *thread;
	GError *error;
	Widgets *widgets = (Widgets*)user_data;

	thread_cond = TRUE;
	gtk_spinner_start (widgets->trans_spinner);
	thread = g_thread_create(thread_trans, (gpointer) widgets, FALSE, &error );
	if(!thread)
	{
		g_print( "Error: %s\n", error->message );
		return;
	}
	if(strcmp(dictionaries[lang_dst].code, "sjp")==0)
	{
		trans_method = 0;
	}
	else if(strcmp(dictionaries[lang_dst].code, "onelook")==0)
	{
		trans_method = 1;
	}
	else
	{
		trans_method = 2;
	}
}


void translate_from_textview (GtkButton *button, gpointer user_data)
{
	GtkTextIter start, end;
	GtkTextBuffer *buffer_src;
    GError *error = NULL;
	gchar *text_to_trans;
	Widgets *widgets = (Widgets*)user_data;

	gdk_threads_add_timeout (100, print_result, (gpointer)user_data);
	buffer_src = gtk_text_view_get_buffer (widgets->src);
	gtk_text_buffer_get_iter_at_offset (buffer_src, &start, 0);
	gtk_text_buffer_get_iter_at_offset (buffer_src, &end, -1);
	text_to_trans = gtk_text_buffer_get_text (buffer_src, 
	                                          &start, &end, FALSE);
	//src_length = strlen(text_to_trans);
	strcpy (recent_clip, text_to_trans);
	//recent_clip[src_length] = '\0';
	
	strncpy(widgets->src_text, text_to_trans, 4096);
	translate (user_data);
	g_free (text_to_trans);
}



void compare_textview_clipboard (GtkClipboard *clipboard, 
                               const char *received_text, gpointer user_data)
{
	GtkTextIter start, end;
	gchar *text_to_trans;
	GtkTextBuffer *buffer_src;
	Widgets *widgets = (Widgets*)user_data;
	
	buffer_src = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widgets->src));

	gtk_text_buffer_get_iter_at_offset (buffer_src, &start, 0);
	gtk_text_buffer_get_iter_at_offset (buffer_src, &end, -1);
	
	text_to_trans = gtk_text_buffer_get_text (buffer_src,
	                                                &start, &end, FALSE);

	if(received_text != NULL && strcmp(text_to_trans, received_text))
	{
		gdk_threads_add_timeout (100, print_result, (gpointer)user_data);
		clipboard_cond = TRUE;
		strncpy(widgets->src_text, received_text, 4096);
		translate (user_data);
	}
	g_free (text_to_trans);
}


void translate_from_clipboard(const char *received_text)
{
	src_length = strlen(received_text);
	strcpy(recent_clip, received_text);
	recent_clip[src_length] = '\0';

	if(strcmp(dictionaries[lang_dst].code, "sjp")==0)
	{
		char *result_trans = getSJP(recent_clip);
		size_t summary_length = strcspn (result_trans, "\n");
		summary = strndup (result_trans, summary_length);
		//translated_text = strdup (result_trans+summary_length+1);
		if(strcmp(result_trans, summary)==0)
		{
			translated_text = '\0';
		}
		else
		{
			translated_text = strdup (result_trans+summary_length+1);
		}
	}
	else if(strcmp(dictionaries[lang_dst].code, "onelook")==0)
	{
		char *result_trans = getOneLook(recent_clip);
		size_t summary_length = strcspn (result_trans, "\n");
		summary = strndup (result_trans, summary_length);
		//translated_text = strdup (result_trans+summary_length+1);
		if(strcmp(result_trans, summary)==0)
		{
			translated_text = '\0';
		}
		else
		{
			translated_text = strdup (result_trans+summary_length+1);
		}
	}
	else
	{
		char *json_out = getTranslation(recent_clip, 
		                                dictionaries[lang_src].code, 
		                                dictionaries[lang_dst].code);
		//g_print("\n%s", json_out);
		char *result_trans = parse_translation(json_out, lang_src, lang_dst);
		//g_print("\n%s", result_trans);
		
		size_t summary_length = strcspn (result_trans, "\n");
		summary = strndup (result_trans, summary_length);	
		if(strcmp(result_trans, summary)==0)
		{
			translated_text = '\0';
		}
		else
		{
			translated_text = strdup (result_trans+summary_length+2);
		}
	}
	if(translated_text != NULL && strlen(translated_text)>700)
	{
		gchar temp_result[750];
		size_t num = 700;
		strncpy(temp_result, translated_text, num);
		temp_result[num] = '\0';
		strcat(temp_result, "... Click to read more\0");
		g_free (translated_text);
		translated_text = strdup (temp_result);
	}
}


void change_src_lang (GtkWidget *widget, gpointer data)
{
	temp = gtk_combo_box_get_active (widget);
	//g_print ("\nactive src = %d", gtk_combo_box_get_active (widget));
	lang_src = temp+successor[temp];
}


void change_dst_lang (GtkWidget *widget, gpointer data)
{
	GdkPixbuf *pixbuffer;
	temp = gtk_combo_box_get_active (widget);
	//g_print ("\nactive dst = %d", gtk_combo_box_get_active (widget));
	lang_dst = temp+successor[temp];
	char icon_file[256];
	if(deploy)
	{
		sprintf (icon_file, "%s/gstranslator/kbflags/%s", PACKAGE_DATA_DIR, dictionaries[lang_dst].flag);
		pixbuffer = gdk_pixbuf_new_from_file(icon_file, NULL);
	}
	else
	{
		sprintf (icon_file, "src/kbflags/%s", dictionaries[lang_dst].flag);
		pixbuffer = gdk_pixbuf_new_from_file(icon_file, NULL);
	}
	notify_notification_set_image_from_pixbuf(notify_open, pixbuffer);
	g_object_unref (pixbuffer);
}


void swap_lang (GtkWidget *widget, gpointer user_data)
{
	Widgets *widgets = (Widgets*)user_data;
	int temp = lang_src;
	lang_src = lang_dst;
	lang_dst = temp;
	gtk_combo_box_set_active (cb_source_lang, dictionaries[lang_src].predecessor);
	gtk_combo_box_set_active (cb_dest_lang, dictionaries[lang_dst].predecessor);
}


gboolean on_key_press (GtkWidget *widget, GdkEventKey *pKey, gpointer user_data)
{
	if (pKey->type == GDK_KEY_PRESS){
		//g_print("%i\n", pKey->keyval);
		switch (pKey->keyval)
		{
			case GDK_KEY_Return:
				translate_from_textview(NULL, user_data);
				break;
		}
	}
	return FALSE;
}


gboolean clean_src_textview (GtkWidget *widget, GdkEventKey* pKey, gpointer user_data)
{
	if (pKey->type == GDK_KEY_RELEASE){
		GtkTextIter start, end;
		GtkTextBuffer *buffer_src;
		Widgets *widgets = (Widgets*)user_data;
		switch (pKey->keyval)
		{
			case GDK_KEY_Return:
				buffer_src = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widgets->src));
				gtk_text_buffer_set_text (buffer_src, widgets->src_text, -1);
				break;
		}
	}
	return FALSE;
}


void init_config (gpointer user_data)
{
	char conf_dir[512]; 
	gchar temp_str[16];
	gint x, y;
	strcpy(conf_dir, getenv("HOME"));
	strcat(conf_dir, "/.config/gstranslator\0");
	strcpy(conf_file, conf_dir);
	strcat(conf_file, "/config.xml\0");

	if(access(conf_file, R_OK)!=0)
	{
		char ch;
		FILE *fs,*ft;
		mkdir(conf_dir, 0755);
		if(deploy)
		{
			fs = fopen(PACKAGE_DATA_DIR"/gstranslator/config/config.xml","r");
		}
		else
		{
			fs = fopen("src/config/config.xml","r");
		}
		if(fs==NULL)
		{
			printf("Cannot open source file. Press key to exit.");
			exit(0);
		}
		ft = fopen(conf_file,"w");
		if(ft==NULL)
		{
			printf("Cannot copy file. Press key to exit.");
			fclose(fs);
			exit(0);
		}

		while(1)
		{
			ch = getc(fs);
			if(ch==EOF)
			{
				break;
			}
			else
				putc(ch,ft);
		}

		fclose(fs);
		fclose(ft);
	}

	Widgets *widgets = (Widgets*)user_data;

	if(!get_xpath_nodes_size (conf_file, "//paned_position", NULL))
	{
		new_child_node (conf_file, "/config", "paned_position", "35");
	}
	if(!get_xpath_nodes_size (conf_file, "//log_filename", NULL))
	{
		char temp_log_filename[512];
		sprintf (temp_log_filename, "%s/log_phrases.xml", conf_dir);
		new_child_node (conf_file, "/config", "log_filename", temp_log_filename);
	}
	if(!get_xpath_nodes_size (conf_file, "//save_frequency", NULL))
	{
		new_child_node (conf_file, "/config", "save_frequency", "5");
	}
	strcpy(temp_str, execute_xpath_expression (conf_file, "//default_width", NULL, 0));
	x = atoi(temp_str);
	strcpy(temp_str, execute_xpath_expression (conf_file, "//default_height", NULL, 0));
	y = atoi(temp_str);
	gtk_window_resize (widgets->window, x, y);
	strcpy(temp_str, execute_xpath_expression (conf_file, "//default_x", NULL, 0));
	x = atoi(temp_str);
	strcpy(temp_str, execute_xpath_expression (conf_file, "//default_y", NULL, 0));
	y = atoi(temp_str);
	gtk_window_move (widgets->window, x, y);
	strcpy(temp_str, execute_xpath_expression (conf_file, "//paned_position", NULL, 0));
	x = atoi(temp_str);
	gtk_paned_set_position (widgets->gstrans_paned, x);
}


void init_languages (gpointer user_data)
{
	char fr[512];
	char line[50];
	int i=0, j;
	char *part, *temp_char;
	size_t cnt=0;
	
	deploy ? sprintf (fr, PACKAGE_DATA_DIR"/gstranslator/config/languages.xml") :
		sprintf (fr, "src/config/languages.xml");
	load_languages_from_xml (fr, &dictionaries, sizeof_dicts);
	//save_languages_to_xml (&dictionaries, sizeof_dicts);
	load_settings (user_data);
}

void load_settings (gpointer user_data)
{
	char *temp_char;
	char lang_temp[128];
	int i, j, k;
	Widgets *widgets = (Widgets*)user_data;

	keybinder_unbind_all (normal_notify_key);
	keybinder_unbind_all (wide_notify_key);
	keybinder_unbind_all (favorite_key);
	keybinder_unbind_all (favorite_key_backward);

	gtk_combo_box_text_remove_all (cb_source_lang);
	gtk_combo_box_text_remove_all (cb_dest_lang);

	
	normal_notify_key = execute_xpath_expression (conf_file, "//normal_notify_hotkey", NULL, 0);
	wide_notify_key = execute_xpath_expression (conf_file, "//wide_notify_hotkey", NULL, 0);
	favorite_key = execute_xpath_expression (conf_file, "//favorite_hotkey", NULL, 0);
	favorite_key_backward = execute_xpath_expression (conf_file, "//favorite_hotkey_back", NULL, 0);

	keybinder_bind (normal_notify_key, normal_notify_handler, user_data);
	keybinder_bind (wide_notify_key, wide_notify_handler, user_data);
	keybinder_bind (favorite_key, change_favorite, user_data);
	keybinder_bind (favorite_key_backward, change_favorite_back, user_data);
	
	favorite_size = get_xpath_nodes_size (conf_file, "//favorite", NULL);
	for(i=0; i<favorite_size; i++)
	{
		sprintf (lang_temp, "//favorite[%d]/src_lang", i+1);
		temp_char = execute_xpath_expression (conf_file, lang_temp, NULL, 0);
		fav_src[i] = atoi (temp_char);
		sprintf (lang_temp, "//favorite[%d]/dst_lang", i+1);
		temp_char = execute_xpath_expression (conf_file, lang_temp, NULL, 0);
		fav_dst[i] = atoi (temp_char);
		//g_print ("\nsrc=%d fav=%d", fav_src[i], fav_dst[i]);
	}

	int size_h = get_xpath_nodes_size (conf_file, "//hidden", NULL);
	int hidden_array[size_h];
	for(i=0; i<size_h; i++)
	{
		sprintf (lang_temp, "//hidden[%d]", i+1);
		temp_char = execute_xpath_expression (conf_file, lang_temp, NULL, 0);
		hidden_array[i] = atoi (temp_char);
	}
	load_settings_log (execute_xpath_expression (conf_file, "/config/save_frequency", NULL, 0),
	                   execute_xpath_expression (conf_file, "/config/log_filename", NULL, 0));

	if(size_h>1)
	{
		qsort (&hidden_array, size_h, sizeof (int), compare_ints);
	}
		
	/*for(i=0; i<size_h; i++)
	{
		g_print("\n%d", hidden_array[i]);
	}*/

	k=0;
	j=0;
	for(i=0; i<60;i++)
	{
		if(i==hidden_array[j])
		{
			j++;
		}
		else
		{
			gtk_combo_box_text_append_text (cb_source_lang, dictionaries[i].name);
			gtk_combo_box_text_append_text (cb_dest_lang, dictionaries[i].name);
			successor[k] = j;
			k++;
		}
		//g_print ("\n%d %s %d=%d %d", i, dictionaries[i].code, k-1, successor[k-1], i-j);
		dictionaries[i].predecessor = i-j;
	}
	favorite_index = -1;
	change_favorite (user_data);
}


void change_favorite (gpointer user_data)
{
	GdkPixbuf *pixbuffer;
	char message[128];
	char icon_file[256];
	Widgets *widgets = (Widgets*)user_data;
	GError *err = NULL;

	favorite_index++;
	if(!(favorite_index<favorite_size))
	{
		favorite_index = 0;
	}
	lang_src = fav_src[favorite_index];
	lang_dst = fav_dst[favorite_index];
	gtk_combo_box_set_active (cb_source_lang, dictionaries[fav_src[favorite_index]].predecessor);
	gtk_combo_box_set_active (cb_dest_lang, dictionaries[fav_dst[favorite_index]].predecessor);

	if(deploy)
	{
		sprintf (icon_file, "%s/gstranslator/kbflags/%s", PACKAGE_DATA_DIR, dictionaries[lang_dst].flag);
		pixbuffer = gdk_pixbuf_new_from_file(icon_file, err);
	}
	else
	{
		sprintf (icon_file, "src/kbflags/%s", dictionaries[lang_dst].flag);
		pixbuffer = gdk_pixbuf_new_from_file(icon_file, err);
	}
	if (err != NULL)
	{
		g_print (stderr, "Unable to read file: %s\n", err->message);
		g_error_free (err);
	}
	notify_notification_set_image_from_pixbuf(notify_open, pixbuffer);
	
	if(!close_notify)
	{
		notify_notification_close (notify_open, NULL);
	}
	close_notify = 1;
	notify_notification_set_urgency (notify_open, NOTIFY_URGENCY_NORMAL);
	//g_print("\n%d %d", lang_src, lang_dst);
	sprintf (message, "%s -> %s", dictionaries[lang_src].name, dictionaries[lang_dst].name);
	notify_notification_update(notify_open, message, "", NULL);
	notify_notification_show (notify_open, NULL);
	recent_clip[0] = "\0";
	g_object_unref (pixbuffer);
}


void change_favorite_back (gpointer user_data)
{
	GdkPixbuf *pixbuffer;
	char message[128];
	char icon_file[256];
	Widgets *widgets = (Widgets*)user_data;

	favorite_index--;
	if(favorite_index<0)
	{
		favorite_index = favorite_size-1;
	}
	lang_src = fav_src[favorite_index];
	lang_dst = fav_dst[favorite_index];
	gtk_combo_box_set_active (cb_source_lang, dictionaries[fav_src[favorite_index]].predecessor);
	gtk_combo_box_set_active (cb_dest_lang, dictionaries[fav_dst[favorite_index]].predecessor);

	if(deploy)
	{
		sprintf (icon_file, "%s/gstranslator/kbflags/%s", PACKAGE_DATA_DIR, dictionaries[lang_dst].flag);
		pixbuffer = gdk_pixbuf_new_from_file(icon_file, NULL);
	}
	else
	{
		sprintf (icon_file, "src/kbflags/%s", dictionaries[lang_dst].flag);
		pixbuffer = gdk_pixbuf_new_from_file(icon_file, NULL);
	}

	notify_notification_set_image_from_pixbuf(notify_open, pixbuffer);
	if(!close_notify)
	{
		notify_notification_close (notify_open, NULL);
	}
	close_notify = 1;
	notify_notification_set_urgency (notify_open, NOTIFY_URGENCY_NORMAL);
	//g_print("\n%d %d", lang_src, lang_dst);
	sprintf (message, "%s -> %s", dictionaries[lang_src].name, dictionaries[lang_dst].name);
	notify_notification_update(notify_open, message, "", NULL);
	notify_notification_show (notify_open, NULL);
	recent_clip[0] = "\0";
	g_object_unref (pixbuffer);
}


void open_properties (GtkMenuItem *menuitem, gpointer user_data)
{
	Widgets *widgets = (Widgets*)user_data;
	save_phrases_to_file ();
	if(create_properties_window(conf_file, deploy, &dictionaries))
	{
		load_settings (user_data);
	}
}


int compare_ints (const void *a, const void *b)
{
	const int *da = (const int *) a;
	const int *db = (const int *) b;

	return (*da > *db) - (*da < *db);
}


void destroy_window (GtkWidget *object, gpointer user_data)
{
	//gtk_widget_hide (object);
	save_size_position (conf_file, &w_width, &w_height, &w_x, &w_y, paned_pos);
	save_phrases_to_file ();
	xmlCleanupParser ();
}


void show_window (GtkStatusIcon *status_icon, GdkEvent *event, gpointer user_data)
{
	Widgets *widgets = (Widgets*)user_data;
	gtk_widget_show_all (widgets->window);
}


void get_size_position (GObject *gobject, GParamSpec *pspec, gpointer user_data)
{
	Widgets *widgets = (Widgets*)user_data;

	paned_pos = gtk_paned_get_position (widgets->gstrans_paned);
	gtk_window_get_size (GTK_WINDOW (widgets->window), &w_width, &w_height);
	gtk_window_get_position (GTK_WINDOW (widgets->window), &w_x, &w_y);
	//g_print ("\n%d %d %d %d", w_width, w_height, w_x, w_y);
	gtk_main_quit ();
}


void get_focus_widget (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	Widgets *widgets = (Widgets*)user_data;
	if(widget==widgets->dest)
	{
		//g_print("\ndest -> focus");
		gtk_widget_set_sensitive (widgets->scan_label, FALSE);
	}
	
}


void open_about_dialog (GtkMenuItem *menuitem, gpointer user_data)
{
	Widgets *widgets = (Widgets*)user_data;
	GdkPixbuf *pixbuffer;
	if(deploy)
	{
		pixbuffer = gdk_pixbuf_new_from_file(PACKAGE_DATA_DIR"/gstranslator/icon/hi64-app-translator.png",
		                                     NULL);
	}
	else
	{
		pixbuffer = gdk_pixbuf_new_from_file("src/icon/hi64-app-translator.png", NULL);
	}
	gtk_about_dialog_set_logo (widgets->about_dialog, pixbuffer);
	gtk_widget_show (widgets->about_dialog);
}