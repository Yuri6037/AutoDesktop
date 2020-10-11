/* main.c
 *
 * Copyright 2020 Yuri Edward
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib/gi18n.h>

#include "autodesktop-config.h"
#include "autodesktop-window.h"

static void on_activate(GtkApplication *app)
{
	GtkWindow *window;

	g_assert(GTK_IS_APPLICATION(app));
	window = gtk_application_get_active_window(app);
	if (window == NULL)
		window = g_object_new(AUTODESKTOP_TYPE_WINDOW,
                              "application", app,
                              "default-width", 600,
                              "default-height", 200,
                              NULL);
    //gtk_window_set_position(window, GTK_WIN_POS_CENTER_ALWAYS); //This is bugged under my GTK install so disabled
    gtk_window_present(window);
}

static gint handle_local_options(G_GNUC_UNUSED GApplication *app, GVariantDict *options)
{
    gchar **filenames;

    if (g_variant_dict_lookup(options, G_OPTION_REMAINING, "^a&ay", &filenames))
    {
        g_variant_dict_insert(options, "filename", "^&ay", filenames[0]);
        g_variant_dict_remove(options, G_OPTION_REMAINING);
        return -1;
    }
    g_printerr("Please specify a file name to create a desktop entry for.\n");
    return EXIT_FAILURE;
}

int main(int argc, char *argv[])
{
	g_autoptr(GtkApplication) app = NULL;
    GOptionEntry additional_entries[] = {
        { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, NULL, "Name of file", "<file to create a desktop entry for>" },
        { NULL, 0, 0, G_OPTION_ARG_NONE, NULL, NULL, NULL }
    };
	int ret;

	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);
	app = gtk_application_new("com.github.yuri6037.AutoDesktop", G_APPLICATION_FLAGS_NONE);
    g_application_add_main_option_entries(G_APPLICATION(app), additional_entries);
	g_signal_connect(app, "handle-local-options", G_CALLBACK(handle_local_options), NULL);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
	ret = g_application_run(G_APPLICATION(app), argc, argv);
	return ret;
}
