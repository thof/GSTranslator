/*
 * properties.c
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

enum
{
   FROM_COLUMN,
   TO_COLUMN,
   N_COLUMNS
};

enum
{
   HIDDEN_COLUMN,
   M_COLUMNS
};

typedef struct
{
	GtkWidget *window;
	GtkWidget *entry_sn;
	GtkWidget *entry_wn;
	GtkWidget *entry_sf;
	GtkWidget *entry_sb;
	GtkWidget *ok_button;
	GtkWidget *cancel_button;
	GtkWidget *move_up_button;
	GtkWidget *move_down_button;
	GtkWidget *delete_button;
	GtkWidget *add_button;
	GtkWidget *delete_hidden_button;
	GtkWidget *add_hidden_button;
	GtkWidget *add_all_hidden_button;
	GtkWidget *store;
	GtkWidget *store_hidden;
	GtkWidget *tree;
	GtkWidget *tree_hidden;
	GtkWidget *column;
	GtkWidget *column_hidden;
	GtkWidget *renderer;
	GtkWidget *renderer_hidden;
	GtkWidget *cb_src;
	GtkWidget *cb_dst;
	GtkWidget *cb_hidden;
} Widgets;

int size_lang, exit_code;
char conf_file[512];
language dicts[60];
shortcuts shortcut[4];
favorites favorite[20];
hidden_dicts h_dict[60];

void init_favorites_list (gpointer user_data, GtkBuilder *builder);
void load_favorites (gpointer user_data);
void init_hidden_list (gpointer user_data, GtkBuilder *builder);
void load_hidden (gpointer user_data);
void move_up (GtkButton *button, gpointer user_data);
void move_down (GtkButton *button, gpointer user_data);
void load_shortcuts (gpointer user_data);
void delete_favorite (GtkButton *button, gpointer user_data);
void add_favorite (GtkButton *button, gpointer user_data);
void delete_hidden (GtkButton *button, gpointer user_data);
void add_hidden (GtkButton *button, gpointer user_data);
void add_all_hidden (GtkButton *button, gpointer user_data);
void save_config (GtkButton *button, gpointer user_data);
void exit_window (GtkButton *button, gpointer user_data);

int create_properties_window(char *conf_file_xml, int deploy, language *dictionaries)
{
	GtkBuilder *builder;
	Widgets *widgets;
	size_lang = 60;
	int i;
	for (i=0; i<size_lang; i++)
	{
		strcpy (dicts[i].name, dictionaries->name);
		dictionaries++;
	}
	strcpy(conf_file, conf_file_xml);
	builder = gtk_builder_new ();
	if(deploy)
	{
		gtk_builder_add_from_file (builder, PACKAGE_DATA_DIR"/gstranslator/ui/properties.ui", NULL);
	}
	else
	{
		gtk_builder_add_from_file (builder, "src/properties.ui", NULL);
	}

	widgets = g_slice_new (Widgets);
	
	widgets->window = gtk_builder_get_object (builder, "properties");
	g_signal_connect (widgets->window, "destroy", G_CALLBACK (exit_window), widgets);
	
	widgets->ok_button = gtk_builder_get_object (builder, "ok_button");
	g_signal_connect (widgets->ok_button, "clicked", G_CALLBACK (save_config), widgets);
	widgets->ok_button = gtk_builder_get_object (builder, "ok_button1");
	g_signal_connect (widgets->ok_button, "clicked", G_CALLBACK (save_config), widgets);
	widgets->ok_button = gtk_builder_get_object (builder, "ok_button2");
	g_signal_connect (widgets->ok_button, "clicked", G_CALLBACK (save_config), widgets);

	widgets->cancel_button = gtk_builder_get_object (builder, "cancel_button");
	g_signal_connect (widgets->cancel_button, "clicked", G_CALLBACK (exit_window), widgets);
	widgets->cancel_button = gtk_builder_get_object (builder, "cancel_button1");
	g_signal_connect (widgets->cancel_button, "clicked", G_CALLBACK (exit_window), widgets);
	widgets->cancel_button = gtk_builder_get_object (builder, "cancel_button2");
	g_signal_connect (widgets->cancel_button, "clicked", G_CALLBACK (exit_window), widgets);

	widgets->entry_sn = gtk_builder_get_object (builder, "entry_sn");
	widgets->entry_wn = gtk_builder_get_object (builder, "entry_wn");
	widgets->entry_sf = gtk_builder_get_object (builder, "entry_sf");
	widgets->entry_sb = gtk_builder_get_object (builder, "entry_sb");

	widgets->move_up_button = gtk_builder_get_object (builder, "move_up_button");
	g_signal_connect (widgets->move_up_button, "clicked", G_CALLBACK (move_up), widgets);
	widgets->move_down_button = gtk_builder_get_object (builder, "move_down_button");
	g_signal_connect (widgets->move_down_button, "clicked", G_CALLBACK (move_down), widgets);
	widgets->cb_src = gtk_builder_get_object (builder, "cb_src");
	widgets->cb_dst = gtk_builder_get_object (builder, "cb_dst");
	widgets->cb_hidden = gtk_builder_get_object (builder, "cb_hidden");
	widgets->add_button = gtk_builder_get_object (builder, "add_button");
	g_signal_connect (widgets->add_button, "clicked", G_CALLBACK (add_favorite), widgets);
	widgets->add_hidden_button = gtk_builder_get_object (builder, "add_hidden_button");
	g_signal_connect (widgets->add_hidden_button, "clicked", G_CALLBACK (add_hidden), widgets);
	widgets->add_all_hidden_button = gtk_builder_get_object (builder, "add_all_hidden_button");
	g_signal_connect (widgets->add_all_hidden_button, "clicked", G_CALLBACK (add_all_hidden), widgets);
	widgets->delete_button = gtk_builder_get_object (builder, "delete_button");
	g_signal_connect (widgets->delete_button, "clicked", G_CALLBACK (delete_favorite), widgets);
	widgets->delete_hidden_button = gtk_builder_get_object (builder, "delete_hidden_button");
	g_signal_connect (widgets->delete_hidden_button, "clicked", G_CALLBACK (delete_hidden), widgets);
	
	load_shortcuts (widgets);
	init_favorites_list (widgets, builder);
	init_hidden_list (widgets, builder);

	gtk_widget_show (widgets->window);
	gtk_main ();

	return exit_code;
}


void init_favorites_list (gpointer user_data, GtkBuilder *builder)
{
	Widgets *widgets = (Widgets*)user_data;
	widgets->store = gtk_tree_store_new (N_COLUMNS,
	                            G_TYPE_STRING,
	                            G_TYPE_STRING);

	load_favorites (widgets);

	widgets->tree = GTK_WIDGET (gtk_builder_get_object (builder, "favorite_view"));
	gtk_tree_view_set_model (widgets->tree, GTK_TREE_MODEL (widgets->store));

	widgets->renderer = gtk_cell_renderer_text_new ();
	widgets->column = gtk_tree_view_column_new_with_attributes ("From", widgets->renderer,
	                                                   "text", FROM_COLUMN, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (widgets->tree), widgets->column);

	widgets->renderer = gtk_cell_renderer_text_new ();
	widgets->column = gtk_tree_view_column_new_with_attributes ("To", widgets->renderer,
	                                                   "text", TO_COLUMN, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (widgets->tree), widgets->column);
}


void load_favorites (gpointer user_data)
{
	GtkTreeIter iter;
	int i, src, dst;
	char lang_temp_src[128], lang_temp_dst[128];
	char *temp_src, *temp_dst;
	Widgets *widgets = (Widgets*)user_data;

	xmlInitParser();
	int favorite_size = get_xpath_nodes_size (conf_file, "//favorite", NULL);
	for(i=0; i<favorite_size; i++)
	{
		sprintf (lang_temp_src, "//favorite[%d]/src_lang", i+1);
		sprintf (lang_temp_dst, "//favorite[%d]/dst_lang", i+1);

		temp_src = execute_xpath_expression (conf_file, lang_temp_src, NULL, 0);
		temp_dst = execute_xpath_expression (conf_file, lang_temp_dst, NULL, 0);
		src = atoi (temp_src);
		dst = atoi (temp_dst);

		gtk_tree_store_append (widgets->store, &iter, NULL);
		gtk_tree_store_set (widgets->store, &iter,
		                    FROM_COLUMN, dicts[src].name,
		                    TO_COLUMN, dicts[dst].name,
		                    -1);
	}
	xmlCleanupParser();

	int size_dicts = strlen(dicts);
	for(i=0; i<size_lang; i++)
	{
		gtk_combo_box_text_append_text (widgets->cb_src, dicts[i].name);
		gtk_combo_box_text_append_text (widgets->cb_dst, dicts[i].name);
		gtk_combo_box_text_append_text (widgets->cb_hidden, dicts[i].name);
	}
}


void init_hidden_list (gpointer user_data, GtkBuilder *builder)
{
	Widgets *widgets = (Widgets*)user_data;
	widgets->store_hidden = gtk_tree_store_new (M_COLUMNS,
	                            G_TYPE_STRING);

	load_hidden (user_data);

	widgets->tree_hidden = GTK_WIDGET (gtk_builder_get_object (builder, "hidden_view"));
	gtk_tree_view_set_model (widgets->tree_hidden, GTK_TREE_MODEL (widgets->store_hidden));

	widgets->renderer_hidden = gtk_cell_renderer_text_new ();
	widgets->column_hidden = gtk_tree_view_column_new_with_attributes ("Dictionary", widgets->renderer_hidden,
	                                                   "text", HIDDEN_COLUMN, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (widgets->tree_hidden), widgets->column_hidden);
}


void load_hidden (gpointer user_data)
{
	GtkTreeIter iter;
	int i, tmp;
	char lang_temp[128];
	char *temp;
	Widgets *widgets = (Widgets*)user_data;

	xmlInitParser();
	int hidden_size = get_xpath_nodes_size (conf_file, "//hidden", NULL);
	for(i=0; i<hidden_size; i++)
	{
		sprintf (lang_temp, "//hidden[%d]", i+1);

		temp = execute_xpath_expression (conf_file, lang_temp, NULL, 0);
		tmp = atoi (temp);

		gtk_tree_store_append (widgets->store_hidden, &iter, NULL);
		gtk_tree_store_set (widgets->store_hidden, &iter,
		                    HIDDEN_COLUMN, dicts[tmp].name,
		                    -1);
	}
	xmlCleanupParser();
}



void move_up (GtkButton *button, gpointer user_data)
{
	GtkTreeIter first_iter, *second_iter;
	GtkTreeSelection *selected;
	Widgets *widgets = (Widgets*)user_data;
	
	selected = gtk_tree_view_get_selection (widgets->tree);
	if(selected == NULL)
	{
		return 0;
	}
	gtk_tree_selection_get_selected (selected, NULL, &first_iter);
	//gtk_tree_store_remove (store, &first_iter);
	second_iter = gtk_tree_iter_copy (&first_iter);
	if(gtk_tree_model_iter_previous (widgets->store, second_iter))
	{
		gtk_tree_store_swap (widgets->store, &first_iter, second_iter);
	}
}


void move_down (GtkButton *button, gpointer user_data)
{
	GtkTreeIter first_iter, *second_iter;
	GtkTreeSelection *selected;
	Widgets *widgets = (Widgets*)user_data;
	
	selected = gtk_tree_view_get_selection (widgets->tree);
	if(selected == NULL)
	{
		return 0;
	}
	gtk_tree_selection_get_selected (selected, NULL, &first_iter);
	//gtk_tree_store_remove (store, &first_iter);
	second_iter = gtk_tree_iter_copy (&first_iter);
	if(gtk_tree_model_iter_next (widgets->store, second_iter))
	{
		gtk_tree_store_swap (widgets->store, &first_iter, second_iter);
	}
}


void delete_favorite (GtkButton *button, gpointer user_data)
{
	GtkTreeIter first_iter;
	GtkTreeSelection *selected;
	Widgets *widgets = (Widgets*)user_data;
	
	selected = gtk_tree_view_get_selection (widgets->tree);
	if(selected == NULL)
	{
		return 0;
	}
	gtk_tree_selection_get_selected (selected, NULL, &first_iter);
	gtk_tree_store_remove (widgets->store, &first_iter);
}


void add_favorite (GtkButton *button, gpointer user_data)
{
	GtkTreeIter iter;
	Widgets *widgets = (Widgets*)user_data;
	
	if(gtk_combo_box_get_active (widgets->cb_src) == -1 || gtk_combo_box_get_active (widgets->cb_dst) == -1)
	{
		return 0;
	}
	
	int src_num = gtk_combo_box_get_active (widgets->cb_src);
	int dst_num = gtk_combo_box_get_active (widgets->cb_dst);
	
	gtk_tree_store_append (widgets->store, &iter, NULL);
	gtk_tree_store_set (widgets->store, &iter,
	                    FROM_COLUMN, dicts[src_num].name,
	                    TO_COLUMN, dicts[dst_num].name,
	                    -1);
}


void delete_hidden (GtkButton *button, gpointer user_data)
{
	GtkTreeIter first_iter;
	GtkTreeSelection *selected;
	Widgets *widgets = (Widgets*)user_data;
	
	selected = gtk_tree_view_get_selection (widgets->tree_hidden);
	if(selected == NULL)
	{
		return 0;
	}
	gtk_tree_selection_get_selected (selected, NULL, &first_iter);
	gtk_tree_store_remove (widgets->store_hidden, &first_iter);

	GtkTreePath *path = gtk_tree_path_new_first ();
	gtk_tree_selection_select_path (selected, path);
}


void add_hidden (GtkButton *button, gpointer user_data)
{
	GtkTreeIter iter;
	gchar *temp_val;
	GValue temp_value = {0};
	Widgets *widgets = (Widgets*)user_data;
	
	if(gtk_combo_box_get_active (widgets->cb_hidden) == -1)
	{
		return 0;
	}
	int num = gtk_combo_box_get_active (widgets->cb_hidden);

	if(gtk_tree_model_get_iter_first (widgets->store_hidden, &iter))
	{
		do
		{
			gtk_tree_model_get_value (widgets->store_hidden, &iter, 0, &temp_value);
			if(strcmp(dicts[num].name, g_value_get_string (&temp_value)) == 0)
			{
				g_value_unset(&temp_value);
				return 0;
			}
			g_value_unset(&temp_value);
		}
		while(gtk_tree_model_iter_next (widgets->store_hidden, &iter));
	}
	
	gtk_tree_store_append (widgets->store_hidden, &iter, NULL);
	gtk_tree_store_set (widgets->store_hidden, &iter,
	                    HIDDEN_COLUMN, dicts[num].name,
	                    -1);
}

void add_all_hidden (GtkButton *button, gpointer user_data)
{
	GtkTreeIter iter;
	int i;	
	Widgets *widgets = (Widgets*)user_data;
	
	gtk_tree_store_clear (widgets->store_hidden);

	for(i=0; i<size_lang; i++)
	{
		gtk_tree_store_append (widgets->store_hidden, &iter, NULL);
		gtk_tree_store_set (widgets->store_hidden, &iter,
		                    HIDDEN_COLUMN, dicts[i].name,
		                    -1);
	}
}


void load_shortcuts (gpointer user_data)
{
	Widgets *widgets = (Widgets*)user_data;
	xmlInitParser();
	gtk_entry_set_text (widgets->entry_sn, execute_xpath_expression (conf_file, "//normal_notify_hotkey", NULL, 0));
	gtk_entry_set_text (widgets->entry_wn, execute_xpath_expression (conf_file, "//wide_notify_hotkey", NULL, 0));
	gtk_entry_set_text (widgets->entry_sf, execute_xpath_expression (conf_file, "//favorite_hotkey", NULL, 0));
	gtk_entry_set_text (widgets->entry_sb, execute_xpath_expression (conf_file, "//favorite_hotkey_back", NULL, 0));
	xmlCleanupParser();
}


void save_config (GtkButton *button, gpointer user_data)
{
	GtkTreeIter f_iter, h_iter;
	GValue f_value = {0};
	GValue h_value = {0};
	char *f_src, *f_dst, *h_val;
	int i, j;
	GtkWidget *dialog;
	Widgets *widgets = (Widgets*)user_data;

	if(gtk_tree_model_get_iter_first (widgets->store, &f_iter) && gtk_tree_model_get_iter_first (widgets->store_hidden, &h_iter))
	{
		gtk_tree_model_get_iter_first (widgets->store_hidden, &h_iter);
		do
		{
			gtk_tree_model_get_value (widgets->store_hidden, &h_iter, 0, &h_value);
			gtk_tree_model_get_iter_first (widgets->store, &f_iter);
			do
			{
				gtk_tree_model_get_value (widgets->store, &f_iter, 0, &f_value);
				if(strcmp(g_value_get_string (&f_value), g_value_get_string (&h_value)) == 0)
				{
					dialog = gtk_message_dialog_new (widgets->window,
					                                 GTK_DIALOG_MODAL,
					                                 GTK_MESSAGE_ERROR,
					                                 GTK_BUTTONS_CLOSE,
					                                 "Both lists contain at least one the same element");
					gtk_message_dialog_format_secondary_text (dialog, "Element \"%s\" needs to be fixed", g_value_get_string (&h_value));
					gtk_dialog_run (GTK_DIALOG (dialog));
					gtk_widget_destroy (dialog);
					g_value_unset(&f_value);
					g_value_unset(&h_value);
					return 0;
				}
				g_value_unset(&f_value);
				
				gtk_tree_model_get_value (widgets->store, &f_iter, 1, &f_value);
				if(strcmp(g_value_get_string (&f_value), g_value_get_string (&h_value)) == 0)
				{
					dialog = gtk_message_dialog_new (widgets->window,
					                                 GTK_DIALOG_MODAL,
					                                 GTK_MESSAGE_ERROR,
					                                 GTK_BUTTONS_CLOSE,
					                                 "Both lists contain at least one the same element");
					gtk_message_dialog_format_secondary_text (dialog, "Element \"%s\" needs to be fixed", g_value_get_string (&h_value));
					gtk_dialog_run (GTK_DIALOG (dialog));
					gtk_widget_destroy (dialog);
					g_value_unset(&f_value);
					g_value_unset(&h_value);
					return 0;
				}
				g_value_unset(&f_value);
			}
			while (gtk_tree_model_iter_next (widgets->store, &f_iter));
			g_value_unset(&h_value);
		}
		while (gtk_tree_model_iter_next (widgets->store_hidden, &h_iter));
	}
	
	if(gtk_tree_model_get_iter_first (widgets->store, &f_iter))
	{
		j=0;
		do
		{
			gtk_tree_model_get_value (widgets->store, &f_iter, 0, &f_value);
			f_src = g_value_get_string (&f_value);
			for(i=0; i<size_lang; i++)
			{
				if(strcmp(f_src, dicts[i].name) == 0)
				{
					favorite[j].src_code = i;
					break;
				}
			}
			g_value_unset(&f_value);
			gtk_tree_model_get_value (widgets->store, &f_iter, 1, &f_value);
			f_dst = g_value_get_string (&f_value);
			for(i=0; i<size_lang; i++)
			{
				if(strcmp(f_dst, dicts[i].name) == 0)
				{
					favorite[j].dst_code = i;
					break;
				}
			}
			g_value_unset(&f_value);
			j++;
		}
		while(gtk_tree_model_iter_next (widgets->store, &f_iter));
	}

	/*for(i=0; i<20; i++)
	{
		if(favorite[i].src_code == NULL)
		{
			break;
		}
		//g_print("\n%d %d", favorite[i].src_code, favorite[i].dst_code);
	}*/

	if(gtk_tree_model_get_iter_first (widgets->store_hidden, &h_iter))
	{
		j=0;
		do
		{
			gtk_tree_model_get_value (widgets->store_hidden, &h_iter, 0, &h_value);
			h_val = g_value_get_string (&h_value);
			for(i=0; i<size_lang; i++)
			{
				if(strcmp(h_val, dicts[i].name) == 0)
				{
					h_dict[j].code = i;
					break;
				}
			}
			g_value_unset(&h_value);
			j++;
		}
		while(gtk_tree_model_iter_next (widgets->store_hidden, &h_iter));
	}

	/*for(i=0; i<60; i++)
	{
		if(h_dict[i].code == NULL)
		{
			break;
		}
		//g_print("\n%d", h_dict[i].code);
	}*/

	strcpy (shortcut[0].name, gtk_entry_get_text (widgets->entry_sn));
	strcpy (shortcut[1].name, gtk_entry_get_text (widgets->entry_wn));
	strcpy (shortcut[2].name, gtk_entry_get_text (widgets->entry_sf));
	strcpy (shortcut[3].name, gtk_entry_get_text (widgets->entry_sb));

	save_config_file (conf_file, &shortcut, &favorite, &h_dict);
	exit_window (NULL, user_data);
	exit_code = 1;
}

void exit_window (GtkButton *button,  gpointer user_data)
{
	Widgets *widgets = (Widgets*)user_data;
	gtk_main_quit ();
	gtk_widget_destroy (widgets->window);
	exit_code = 0;
}