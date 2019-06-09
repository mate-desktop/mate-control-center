/*
 *  Authors: Luca Cavalli <loopback@slackit.org>
 *
 *  Copyright 2005-2006 Luca Cavalli
 *  Copyright 2008 Thomas Wood <thos@gnome.org>
 *  Copyright 2010 Perberos <perberos@gmail.com>
 *  Copyright 2019 Laurent Napias
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of version 2 of the GNU General Public License
 *  as published by the Free Software Foundation
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02110-1301, USA.
 *
 */

#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif

#include <string.h>
#include <glib/gi18n.h>
#include <stdlib.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>

#include "mate-di-capplet.h"
#include "capplet-util.h"

#define get_widget(name) GTK_WIDGET(gtk_builder_get_object(builder, name))

static void
close_cb(GtkWidget* window, gint response, MateDICapplet* capplet)
{
	gtk_widget_destroy(window);
	gtk_main_quit();
}

/* Show or hide all desktop icons */
static void
show_icons_cb(GtkCheckButton* button, MateDICapplet* capplet)
{
	gboolean show_icons = gtk_toggle_button_get_active(button);
	
	gtk_widget_set_sensitive(capplet->desktop_icons_caption, show_icons);

	gtk_widget_set_sensitive(GTK_WIDGET (capplet->hbox_computer) , show_icons);
	gtk_widget_set_sensitive(GTK_WIDGET (capplet->hbox_home) , show_icons);
	gtk_widget_set_sensitive(GTK_WIDGET (capplet->hbox_trash) , show_icons);
	gtk_widget_set_sensitive(GTK_WIDGET (capplet->hbox_network) , show_icons);

	gtk_widget_set_sensitive(GTK_WIDGET (capplet->button_volumes_toggle) , show_icons);

}

/* Enter edit icon name for computer icon */
static void
edit_computer_icon_name_cb(GtkWidget* button, MateDICapplet* capplet)
{
	/* We enter in edit mode so change GUI to enter new name */
	if (strcmp( gtk_button_get_label(button), "gtk-edit" ) == 0 )
	{
		gtk_widget_hide(capplet->button_computer_toggle);
		gtk_entry_set_text(capplet->entry_computer, gtk_button_get_label(capplet->button_computer_toggle) );
		gtk_widget_show(capplet->entry_computer);
		gtk_button_set_label(GTK_BUTTON(button), "gtk-undo");
	}
	else
	{
		/* Leaving edit mode and set icon name to default value */
		g_settings_reset(capplet->desktop_icons_settings, COMPUTER_NAME_KEY);
		
		gtk_button_set_label(GTK_BUTTON(capplet->button_computer_toggle), _("Computer"));
		gtk_widget_show(capplet->button_computer_toggle);
		gtk_widget_hide(capplet->entry_computer);
		gtk_button_set_label(GTK_BUTTON(button), "gtk-edit");
	}
}

/* Leave edit icon name mode and save new icon name for computer icon */
void
on_activate_entry_computer_cb(GtkWidget *entry, MateDICapplet* capplet)
{
    const gchar* icon_name = gtk_entry_get_text(GTK_ENTRY(entry));

	g_settings_set_string(capplet->desktop_icons_settings, COMPUTER_NAME_KEY, icon_name);
 
    gtk_button_set_label(GTK_BUTTON(capplet->button_computer_toggle), icon_name);

	gtk_widget_show(capplet->button_computer_toggle);
	gtk_widget_hide(capplet->entry_computer);
	gtk_button_set_label(GTK_BUTTON(capplet->button_edit_computer), "gtk-edit");
}

/* Enter edit icon name for home icon */
static void
edit_home_icon_name_cb(GtkWidget* button, MateDICapplet* capplet)
{
	/* We enter in edit mode so change GUI to enter new name */
	if (strcmp( gtk_button_get_label(button), "gtk-edit" ) == 0 )
	{
		gtk_widget_hide(capplet->button_home_toggle);
		gtk_entry_set_text(capplet->entry_home, gtk_button_get_label(capplet->button_home_toggle) );
		gtk_widget_show(capplet->entry_home);
		gtk_button_set_label(GTK_BUTTON(button), "gtk-undo");
	}
	else
	{
		/* Leaving edit mode and set icon name to default value */
		g_settings_reset(capplet->desktop_icons_settings, HOME_NAME_KEY);
		
		gtk_button_set_label(GTK_BUTTON(capplet->button_home_toggle), _("Home"));
		gtk_widget_show(capplet->button_home_toggle);
		gtk_widget_hide(capplet->entry_home);
		gtk_button_set_label(GTK_BUTTON(button), "gtk-edit");
	}
}

/* Leave edit icon name mode and save new icon name for home icon */
void
on_activate_entry_home_cb(GtkWidget *entry, MateDICapplet* capplet)
{
    const gchar *icon_name = gtk_entry_get_text(GTK_ENTRY(entry));

	g_settings_set_string(capplet->desktop_icons_settings, HOME_NAME_KEY, icon_name);
 
    gtk_button_set_label(GTK_BUTTON(capplet->button_home_toggle), icon_name);

	gtk_widget_show(capplet->button_home_toggle);
	gtk_widget_hide(capplet->entry_home);
	gtk_button_set_label(GTK_BUTTON(capplet->button_edit_home), "gtk-edit");
}

/* Enter edit icon name for trash icon */
static void
edit_trash_icon_name_cb(GtkWidget* button, MateDICapplet* capplet)
{
	/* We enter in edit mode so change GUI to enter new name */
	if (strcmp( gtk_button_get_label(button), "gtk-edit" ) == 0 )
	{
		gtk_widget_hide(capplet->button_trash_toggle);
		gtk_entry_set_text(capplet->entry_trash, gtk_button_get_label(capplet->button_trash_toggle) );
		gtk_widget_show(capplet->entry_trash);
		gtk_button_set_label(GTK_BUTTON(button), "gtk-undo");
	}
	else
	{
		/* Leaving edit mode and set icon name to default value */
		g_settings_reset(capplet->desktop_icons_settings, TRASH_NAME_KEY);
		
		gtk_button_set_label(GTK_BUTTON(capplet->button_trash_toggle), _("Trash"));
		gtk_widget_show(capplet->button_trash_toggle);
		gtk_widget_hide(capplet->entry_trash);
		gtk_button_set_label(GTK_BUTTON(button), "gtk-edit");
	}
}

/* Leave edit icon name mode and save new icon name for trash icon */
void
on_activate_entry_trash_cb(GtkWidget *entry, MateDICapplet* capplet)
{
    const gchar *icon_name = gtk_entry_get_text(GTK_ENTRY(entry));

	g_settings_set_string(capplet->desktop_icons_settings, TRASH_NAME_KEY, icon_name);
 
    gtk_button_set_label(GTK_BUTTON(capplet->button_trash_toggle), icon_name);

	gtk_widget_show(capplet->button_trash_toggle);
	gtk_widget_hide(capplet->entry_trash);
	gtk_button_set_label(GTK_BUTTON(capplet->button_edit_trash), "gtk-edit");
}

/* Enter edit icon name for network icon */
static void
edit_network_icon_name_cb(GtkWidget* button, MateDICapplet* capplet)
{
	/* We enter in edit mode so change GUI to enter new name */
	if (strcmp( gtk_button_get_label(button), "gtk-edit" ) == 0 )
	{
		gtk_widget_hide(capplet->button_network_toggle);
		gtk_entry_set_text(capplet->entry_network, gtk_button_get_label(capplet->button_network_toggle) );
		gtk_widget_show(capplet->entry_network);
		gtk_button_set_label(GTK_BUTTON(button), "gtk-undo");
	}
	else
	{
		/* Leaving edit mode and set icon name to default value */
		g_settings_reset(capplet->desktop_icons_settings, NETWORK_NAME_KEY);
		
		gtk_button_set_label(GTK_BUTTON(capplet->button_network_toggle), _("NetWork"));
		gtk_widget_show(capplet->button_network_toggle);
		gtk_widget_hide(capplet->entry_network);
		gtk_button_set_label(GTK_BUTTON(button), "gtk-edit");
	}
}

/* Leave edit icon name mode and save new icon name for network icon */
void
on_activate_entry_network_cb(GtkWidget *entry, MateDICapplet* capplet)
{
    const gchar *icon_name = gtk_entry_get_text(GTK_ENTRY(entry));

	g_settings_set_string(capplet->desktop_icons_settings, NETWORK_NAME_KEY, icon_name);
 
    gtk_button_set_label(GTK_BUTTON(capplet->button_network_toggle), icon_name);

	gtk_widget_show(capplet->button_network_toggle);
	gtk_widget_hide(capplet->entry_network);
	gtk_button_set_label(GTK_BUTTON(capplet->button_edit_network), "gtk-edit");
}

/* Get default icons names set by the user */
void
get_user_icons_names(MateDICapplet* capplet)
{
	/* Get computer icon name if set by the user */
	g_autoptr(GVariant) *user_computer_name = g_settings_get_user_value(capplet->desktop_icons_settings, COMPUTER_NAME_KEY);
	if (user_computer_name != NULL)
	{
		gchar *computer_name = g_settings_get_string(capplet->desktop_icons_settings, COMPUTER_NAME_KEY);
		gtk_button_set_label(GTK_BUTTON(capplet->button_computer_toggle), computer_name);
		g_free(computer_name);
	}
	g_variant_unref(user_computer_name);

	/* Get home icon name if set by the user */
	g_autoptr(GVariant) *user_home_name = g_settings_get_user_value(capplet->desktop_icons_settings, HOME_NAME_KEY);
	if (user_home_name != NULL)
	{
		gchar *home_name = g_settings_get_string(capplet->desktop_icons_settings, HOME_NAME_KEY);
		gtk_button_set_label(GTK_BUTTON(capplet->button_home_toggle), home_name);
		g_free(home_name);
	}
	g_variant_unref(user_home_name);

	/* Get trash icon name if set by the user */
	g_autoptr(GVariant) *user_trash_name = g_settings_get_user_value(capplet->desktop_icons_settings, TRASH_NAME_KEY);
	if (user_trash_name != NULL)
	{
		gchar *trash_name = g_settings_get_string(capplet->desktop_icons_settings, TRASH_NAME_KEY);
		gtk_button_set_label(GTK_BUTTON(capplet->button_trash_toggle), trash_name);
		g_free(trash_name);
	}
	g_variant_unref(user_trash_name);

	/* Get network icon name if set by the user */
	g_autoptr(GVariant) *user_network_name = g_settings_get_user_value(capplet->desktop_icons_settings, NETWORK_NAME_KEY);
	if (user_network_name != NULL)
	{
		gchar *network_name = g_settings_get_string(capplet->desktop_icons_settings, NETWORK_NAME_KEY);
		gtk_button_set_label(GTK_BUTTON(capplet->button_network_toggle), network_name);
		g_free(network_name);
	}
	g_variant_unref(user_network_name);
}

/* Init GUI reading user preferences if set */
static void
init_dialog(MateDICapplet* capplet)
{
	GtkBuilder* builder;
	guint builder_result;

	capplet->builder = builder = gtk_builder_new ();

	builder_result = gtk_builder_add_from_resource (builder, "/org/mate/mcc/di/mate-desktop-icons.ui", NULL);

	if (builder_result == 0)
	{
		GtkWidget* dialog = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Could not load the main interface"));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), _("Please make sure that the applet is properly installed"));
		gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

		gtk_dialog_run(GTK_DIALOG(dialog));

		gtk_widget_destroy(dialog);
		exit(EXIT_FAILURE);
	}

	capplet->window = get_widget("desktop_icons_dialog");

	/* Settings */
	capplet->show_icons_settings = g_settings_new (SHOW_ICONS_SCHEMA);
	capplet->desktop_icons_settings = g_settings_new (DESKTOP_ICONS_SCHEMA);

	capplet->button_show_icons_toggle = get_widget("button_show_icons_toggle");
	capplet->desktop_icons_caption = get_widget("desktop_icons_caption");

	/* CheckButton to allow icon showing or not */
	gtk_toggle_button_set_active(capplet->button_show_icons_toggle, g_settings_get_boolean(capplet->show_icons_settings, SHOW_ICONS_KEY));

	capplet->desktop_icons_caption = get_widget("desktop_icons_caption");

	/* One CheckButton for each icon to show or hide*/
	capplet->button_computer_toggle = get_widget("button_computer_toggle");
	capplet->button_home_toggle = get_widget("button_home_toggle");
	capplet->button_trash_toggle = get_widget("button_trash_toggle");
	capplet->button_network_toggle = get_widget("button_network_toggle");
	capplet->button_volumes_toggle = get_widget("button_volumes_toggle");

	/* Get default icons names */
	get_user_icons_names(capplet);

	/* One GtkBox per icon to show/hide or rename */
	capplet->hbox_computer = get_widget("hbox_computer");
	capplet->hbox_home = get_widget("hbox_home");
	capplet->hbox_trash = get_widget("hbox_trash");
	capplet->hbox_network = get_widget("hbox_network");

	/* To enter new name for icons */
	capplet->entry_computer = get_widget("entry_computer");
	capplet->entry_home = get_widget("entry_home");
	capplet->entry_trash = get_widget("entry_trash");
	capplet->entry_network = get_widget("entry_network");

	/* To rename icon or set to default name */
	capplet->button_edit_computer = get_widget("button_edit_computer");
	capplet->button_edit_home = get_widget("button_edit_home");
	capplet->button_edit_trash = get_widget("button_edit_trash");
	capplet->button_edit_network = get_widget("button_edit_network");

	/* Signals */
	g_signal_connect(capplet->window, "response", G_CALLBACK(close_cb), capplet);
	
	/* Enter edit icon name and validate new name for computer icon */
	g_signal_connect(capplet->button_edit_computer, "clicked", G_CALLBACK (edit_computer_icon_name_cb), capplet);
	g_signal_connect(capplet->entry_computer, "activate", G_CALLBACK(on_activate_entry_computer_cb), capplet);

	/* Enter edit icon name and validate new name for home icon */
	g_signal_connect(capplet->button_edit_home, "clicked", G_CALLBACK (edit_home_icon_name_cb), capplet);
	g_signal_connect(capplet->entry_home, "activate", G_CALLBACK(on_activate_entry_home_cb), capplet);

	/* Enter edit icon name and validate new name for trash icon */
	g_signal_connect(capplet->button_edit_trash, "clicked", G_CALLBACK (edit_trash_icon_name_cb), capplet);
	g_signal_connect(capplet->entry_trash, "activate", G_CALLBACK(on_activate_entry_trash_cb), capplet);

	/* Enter edit icon name and validate new name for network icon */
	g_signal_connect(capplet->button_edit_network, "clicked", G_CALLBACK (edit_network_icon_name_cb), capplet);
	g_signal_connect(capplet->entry_network, "activate", G_CALLBACK(on_activate_entry_network_cb), capplet);

	/* Show or hide desktop icons */
	g_signal_connect(capplet->button_show_icons_toggle, "toggled", G_CALLBACK (show_icons_cb), capplet);

	/* Change settings showing or hiding a desktop icon */
    g_settings_bind(capplet->show_icons_settings, SHOW_ICONS_KEY, capplet->button_show_icons_toggle, "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind(capplet->desktop_icons_settings, COMPUTER_ICON_KEY, capplet->button_computer_toggle, "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind(capplet->desktop_icons_settings, HOME_ICON_KEY, capplet->button_home_toggle, "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind(capplet->desktop_icons_settings, TRASH_ICON_KEY, capplet->button_trash_toggle, "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind(capplet->desktop_icons_settings, NETWORK_ICON_KEY, capplet->button_network_toggle, "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind(capplet->desktop_icons_settings, VOLUMES_ICON_KEY, capplet->button_volumes_toggle, "active", G_SETTINGS_BIND_DEFAULT);

	gtk_window_set_icon_name(GTK_WINDOW (capplet->window), "user-desktop");
}

int
main(int argc, char** argv)
{
	GOptionContext* context = g_option_context_new(_("- MATE Default Applications"));

	capplet_init(context, &argc, &argv);

	MateDICapplet* capplet = g_new0(MateDICapplet, 1);

	init_dialog(capplet);
	gtk_widget_show(capplet->window);

	gtk_main();
	
	/* free stuff */
	g_free(capplet);

	return EXIT_SUCCESS;
}
