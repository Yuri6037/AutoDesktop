/* autodesktop-window.c
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
#include <glib/gstdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "autodesktop-config.h"
#include "autodesktop-window.h"

struct _AutodesktopWindow
{
    GtkApplicationWindow parent_instance;

    GtkButton *button_cancel;
    GtkButton *button_create;
    GtkButton *button_icon;
    GtkImage *desktop_icon;
    GtkEntry *desktop_name;
    GtkEntry *desktop_exec;
    gchar *icon_filename;
};

#define PROP_FILENAME 1

static GParamSpec *obj_properties[2] = { NULL, NULL };

G_DEFINE_TYPE(AutodesktopWindow, autodesktop_window, GTK_TYPE_APPLICATION_WINDOW)

static void autodesktop_window_finalize(GObject *obj)
{
    g_free(AUTODESKTOP_WINDOW(obj)->icon_filename);
}

static gchar *guess_content_type_read(const gchar *filename)
{
    guchar buf[512];
    int fd;
    int len;

    fd = g_open(filename, O_RDONLY, 0);
    if (fd == -1)
        return (g_strdup("application/octet-stream"));
    len = read(fd, buf, 512);
    close(fd);
    return (g_content_type_guess(filename, buf, len, NULL));
}

static gchar *guess_content_type(const gchar *filename)
{
    gboolean uncertain = FALSE;
    gchar *data;

    if (g_file_test(filename, G_FILE_TEST_IS_DIR))
        return (g_strdup("inode/directory"));
    data = g_content_type_guess(filename, NULL, 0, &uncertain);
    if (uncertain)
    {
        g_free(data);
        return guess_content_type_read(filename);
    }
    else
        return (data);
}

static void prepend_set(AutodesktopWindow *self, const gchar *prepend, const gchar *data)
{
    g_autofree gchar *tmp = g_strjoin(" ", prepend, data, NULL);

    gtk_entry_set_text(self->desktop_exec, tmp);
}

static void autodesktop_window_set_property(GObject *obj, guint prop, const GValue *value, GParamSpec *pspec)
{
    AutodesktopWindow *self = AUTODESKTOP_WINDOW(obj);
    const gchar *data;
    g_autofree gchar *content_type = NULL;

    switch (prop)
    {
    case PROP_FILENAME:
        if (g_value_get_string(value) != NULL)
        {
            data = g_value_get_string(value);
            content_type = guess_content_type(data);
            if (g_strcmp0(content_type, "application/x-executable") == 0
                || g_strcmp0(content_type, "application/x-shellscript") == 0
                || g_strcmp0(content_type, "application/vnd.appimage") == 0)
                gtk_entry_set_text(self->desktop_exec, data);
            else if (g_strcmp0(content_type, "inode/directory") == 0)
                prepend_set(self, "nautilus", data);
            else if (g_strcmp0(content_type, "text/plain") == 0)
                prepend_set(self, "gedit", data);
            else
                prepend_set(self, "bless", data);
            g_debug("Init target command: %s", data);
        }
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop, pspec);
        break;
    }
}

static void autodesktop_window_get_property(GObject *obj, guint prop, GValue *value, GParamSpec *pspec)
{
    AutodesktopWindow *self = AUTODESKTOP_WINDOW(obj);

    switch (prop)
    {
    case PROP_FILENAME:
        g_value_set_static_string(value, gtk_entry_get_text(self->desktop_exec));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop, pspec);
        break;
    }
}

static void autodesktop_window_class_init(AutodesktopWindowClass *class)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(class);

    gtk_widget_class_set_template_from_resource(widget_class, "/com/github/yuri6037/AutoDesktop/autodesktop-window.ui");
    gtk_widget_class_bind_template_child(widget_class, AutodesktopWindow, button_cancel);
    gtk_widget_class_bind_template_child(widget_class, AutodesktopWindow, button_create);
    gtk_widget_class_bind_template_child(widget_class, AutodesktopWindow, button_icon);
    gtk_widget_class_bind_template_child(widget_class, AutodesktopWindow, desktop_icon);
    gtk_widget_class_bind_template_child(widget_class, AutodesktopWindow, desktop_name);
    gtk_widget_class_bind_template_child(widget_class, AutodesktopWindow, desktop_exec);
    G_OBJECT_CLASS(class)->finalize = autodesktop_window_finalize;
    G_OBJECT_CLASS(class)->set_property = autodesktop_window_set_property;
    G_OBJECT_CLASS(class)->get_property = autodesktop_window_get_property;
    obj_properties[PROP_FILENAME] = g_param_spec_string(
        "filename",
        "Filename",
        "Filename of target executable",
        NULL,
        G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE
    );
    g_object_class_install_properties(G_OBJECT_CLASS(class), 2, obj_properties);
}

static void show_message_box(GtkWidget *widget)
{
    gtk_dialog_run(GTK_DIALOG(widget));
    gtk_widget_destroy(widget);
}

static void autodesktop_window_button_icon_clicked(G_GNUC_UNUSED GtkButton *button, AutodesktopWindow *self)
{
    GtkFileFilter *filter = gtk_file_filter_new();
    GtkWidget *dialog;
    gint res;
    GError *err = NULL;
    GdkPixbuf *pixbuf;

    dialog = gtk_file_chooser_dialog_new(_("Choose Icon"), GTK_WINDOW(self),
                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                         _("_Cancel"), GTK_RESPONSE_CANCEL,
                                         _("_Open"), GTK_RESPONSE_ACCEPT,
                                         NULL);
    gtk_file_filter_set_name(filter, _("Compatible Image Files"));
    gtk_file_filter_add_mime_type(filter, "image/jpeg");
    gtk_file_filter_add_mime_type(filter, "image/png");
    gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
    res = gtk_dialog_run(GTK_DIALOG(dialog));
    if (res == GTK_RESPONSE_ACCEPT)
    {
        self->icon_filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        g_debug("User chose new icon file: '%s'", self->icon_filename);
        pixbuf = gdk_pixbuf_new_from_file_at_scale(self->icon_filename, 128, 128, TRUE, &err);
        if (err != NULL)
        {
            show_message_box(gtk_message_dialog_new(GTK_WINDOW(self),
                                                    GTK_DIALOG_MODAL,
                                                    GTK_MESSAGE_ERROR,
                                                    GTK_BUTTONS_OK,
                                                    "An error has occured while setting the image:\n%s\n",
                                                    err->message));
            g_error_free(err);
        }
        else
            gtk_image_set_from_pixbuf(self->desktop_icon, pixbuf);
    }
    gtk_widget_destroy(dialog);
}

static void create_desktop_entry(AutodesktopWindow *self, const gchar *name, const gchar *exec)
{
    g_autofree gchar *filename = g_strjoin(".", name, "desktop", NULL);
    g_autofree gchar *path = g_strjoin("/", g_get_home_dir(), ".local", "share", "applications", filename, NULL);
    g_autofree gchar *rmcmd = g_strjoin("", "rm", " ", "\"", path, "\"", NULL);
    GKeyFile *file = g_key_file_new();
    GError *err = NULL;

    g_key_file_set_string(file, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_TYPE, "Application");
    g_key_file_set_string(file, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_EXEC, exec);
    if (self->icon_filename != NULL)
        g_key_file_set_string(file, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_ICON, self->icon_filename);
    g_key_file_set_string(file, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_NAME, name);
    g_key_file_set_string(file, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_ACTIONS, "Remove;");
    g_key_file_set_string(file, "Desktop Action Remove", G_KEY_FILE_DESKTOP_KEY_NAME, "Remove from Launcher");
    g_key_file_set_string(file, "Desktop Action Remove", G_KEY_FILE_DESKTOP_KEY_EXEC, rmcmd);
    if (!g_key_file_save_to_file(file, path, &err))
    {
        show_message_box(gtk_message_dialog_new(GTK_WINDOW(self),
                                                GTK_DIALOG_MODAL,
                                                GTK_MESSAGE_ERROR,
                                                GTK_BUTTONS_OK,
                                                "An error has occured while saving desktop entry:\n%s\n",
                                                err->message));
        g_error_free(err);
    }
    else
        gtk_window_close(GTK_WINDOW(self));
}

static void autodesktop_window_button_create_clicked(G_GNUC_UNUSED GtkButton *button, AutodesktopWindow *self)
{
    const gchar *name = gtk_entry_get_text(self->desktop_name);
    const gchar *exec = gtk_entry_get_text(self->desktop_exec);

    if (name == NULL || exec == NULL || strlen(name) == 0 || strlen(exec) == 0)
        show_message_box(gtk_message_dialog_new(GTK_WINDOW(self),
                                                GTK_DIALOG_MODAL,
                                                GTK_MESSAGE_ERROR,
                                                GTK_BUTTONS_OK,
                                                "Please specify a name and a command line\n"));
    else
    {
        g_debug("Creating new desktop entry...");
        create_desktop_entry(self, name, exec);
    }
}

static void autodesktop_window_button_cancel_clicked(G_GNUC_UNUSED GtkButton *button, AutodesktopWindow *self)
{
    g_debug("User requested cancel operation, exiting...");
    gtk_window_close(GTK_WINDOW(self));
}

static void autodesktop_window_init(AutodesktopWindow *self)
{
    self->icon_filename = NULL;
    gtk_widget_init_template(GTK_WIDGET(self));
    g_signal_connect(self->button_icon, "clicked", G_CALLBACK(autodesktop_window_button_icon_clicked), self);
    g_signal_connect(self->button_create, "clicked", G_CALLBACK(autodesktop_window_button_create_clicked), self);
    g_signal_connect(self->button_cancel, "clicked", G_CALLBACK(autodesktop_window_button_cancel_clicked), self);
}
