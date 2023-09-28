/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 *
 * Copyright (C) 2023 MATE Developers
 * Copyright (C) 2019 Purism SPC
 * Copyright (C) 2017 Mohammed Sadiq <sadiq@sadiqpk.org>
 * Copyright (C) 2010 Red Hat, Inc
 * Copyright (C) 2008 William Jon McCann <jmccann@redhat.com>
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <gtk/gtk.h>
#include <glibtop/fsusage.h>
#include <glibtop/mountlist.h>
#include <glibtop/mem.h>
#include <glibtop/sysinfo.h>
#include <udisks/udisks.h>
#include <sys/utsname.h>

#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/gdkwayland.h>
#endif
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif

#include "info-cleanup.h"
#include "mate-system-info.h"
#include "mate-system-info-resources.h"

struct _MateSystemInfo
{
    GtkDialog    parent_instance;
    GtkWidget   *logo_image;
    GtkWidget   *hostname_row;
    GtkListBox  *hardware_box;
    GtkWidget   *hardware_model_row;
    GtkWidget   *memory_row;
    GtkWidget   *processor_row;
    GtkWidget   *graphics_row;
    GtkWidget   *disk_row;
    GtkListBox  *os_box;
    GtkWidget   *kernel_row;
    GtkWidget   *virtualization_row;
    GtkWidget   *windowing_system_row;
    GtkWidget   *mate_version_row;
    GtkWidget   *os_name_row;
    GtkWidget   *os_type_row;
};

G_DEFINE_TYPE (MateSystemInfo, mate_system_info, GTK_TYPE_DIALOG)

static void
set_lable_style (GtkWidget  *lable,
                 const char *color,
                 int         font_szie,
                 const char *text,
                 gboolean    blod)
{
    g_autofree gchar *lable_text = NULL;

    if (color == NULL)
    {
        lable_text = g_strdup_printf ("<span weight=\'light\'font_desc=\'%d\'><b>%s</b></span>", font_szie, text);
    }
    else
    {
        if(blod)
        {
            lable_text = g_strdup_printf ("<span foreground=\'%s\'weight=\'light\'font_desc=\'%d\'><b>%s</b></span>",
                             color,
                             font_szie,
                             text);
        }
        else
        {
            lable_text = g_strdup_printf ("<span foreground=\'%s\'weight=\'light\'font_desc=\'%d\'>%s</span>",
                            color,
                            font_szie,
                            text);
        }
    }

    gtk_label_set_markup (GTK_LABEL(lable), lable_text);
}

static void
mate_system_info_row_fill (GtkWidget  *row,
                           const char *labelname,
                           gboolean    is_separator)
{
    GtkWidget  *vbox;
    GtkWidget  *box;
    GtkWidget  *label;
    GtkWidget  *separator;

    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_set_spacing (GTK_BOX (box), 12);
    g_object_set (box, "margin", 12, NULL);
    gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, FALSE, 3);

    label = gtk_label_new (NULL);
    set_lable_style (label, NULL, 12, labelname, TRUE);
    gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 6);

    label = gtk_label_new (NULL);
    gtk_box_pack_end (GTK_BOX (box), label, FALSE, FALSE, 6);
    g_object_set_data (G_OBJECT (row), "labelvalue", label);

    separator = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
    if (is_separator)
        gtk_box_pack_start (GTK_BOX (vbox), separator, FALSE, FALSE, 0);

    gtk_container_add (GTK_CONTAINER (row), vbox);
}

static void
mate_system_info_set_row (MateSystemInfo *info)
{
    mate_system_info_row_fill (info->hostname_row, _("Device Name"), FALSE);
    mate_system_info_row_fill (info->hardware_model_row, _("Hardware Model"), TRUE);
    mate_system_info_row_fill (info->memory_row, _("Memory"), TRUE);
    mate_system_info_row_fill (info->processor_row, _("Processor"), TRUE);
    mate_system_info_row_fill (info->graphics_row, _("Graphics"), TRUE);
    mate_system_info_row_fill (info->disk_row, _("Disk Capacity"), FALSE);
    mate_system_info_row_fill (info->kernel_row, _("Kernel Version"), FALSE);
    mate_system_info_row_fill (info->virtualization_row, _("Virtualization"), TRUE);
    mate_system_info_row_fill (info->windowing_system_row, _("Windowing System"), TRUE);
    mate_system_info_row_fill (info->mate_version_row, _("MATE Version"), TRUE);
    mate_system_info_row_fill (info->os_name_row, _("OS Name"), TRUE);
    mate_system_info_row_fill (info->os_type_row, _("OS Type"), TRUE);

    gtk_widget_show (info->logo_image);
    gtk_widget_show_all (info->hostname_row);
    gtk_widget_show_all (info->memory_row);
    gtk_widget_show_all (info->processor_row);
    gtk_widget_show_all (info->graphics_row);
    gtk_widget_show_all (info->disk_row);
    gtk_widget_show_all (info->kernel_row);
    gtk_widget_show_all (info->windowing_system_row);
    gtk_widget_show_all (info->mate_version_row);
    gtk_widget_show_all (info->os_type_row);
    gtk_widget_show_all (info->os_name_row);
}

static char *
get_system_hostname (void)
{
    GDBusProxy         *hostnamed_proxy;
    g_autoptr(GVariant) variant = NULL;
    g_autoptr(GError)   error = NULL; 

    hostnamed_proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                                                     G_DBUS_PROXY_FLAGS_NONE,
                                                     NULL,
                                                     "org.freedesktop.hostname1",
                                                     "/org/freedesktop/hostname1",
                                                     "org.freedesktop.hostname1",
                                                     NULL,
                                                     &error);

    if (!hostnamed_proxy)
        return g_strdup ("");

    variant = g_dbus_proxy_get_cached_property (hostnamed_proxy, "Hostname");
    if (!variant)
    {
        g_autoptr(GError) error = NULL;
        g_autoptr(GVariant) inner = NULL;

      /* Work around systemd-hostname not sending us back
       * the property value when changing values */
        variant = g_dbus_proxy_call_sync (hostnamed_proxy,
                                         "org.freedesktop.DBus.Properties.Get",
                                          g_variant_new ("(ss)", "org.freedesktop.hostname1", "Hostname"),
                                          G_DBUS_CALL_FLAGS_NONE,
                                          -1,
                                          NULL,
                                          &error);
        if (variant == NULL)
        {
            g_warning ("Failed to get property '%s': %s", "Hostname", error->message);
            g_object_unref (hostnamed_proxy);
            return NULL;
        }

        g_variant_get (variant, "(v)", &inner);
        g_object_unref (hostnamed_proxy);
        return g_variant_dup_string (inner, NULL);
    }
    else
    {
        g_object_unref (hostnamed_proxy);
        return g_variant_dup_string (variant, NULL);
    }
}

static char *
get_hardware_model (void)
{
    g_autoptr(GDBusProxy) hostnamed_proxy = NULL;
    g_autoptr(GVariant) vendor_variant = NULL;
    g_autoptr(GVariant) model_variant = NULL;
    const char *vendor_string, *model_string;
    g_autoptr(GError) error = NULL;

    hostnamed_proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                                                     G_DBUS_PROXY_FLAGS_NONE,
                                                     NULL,
                                                     "org.freedesktop.hostname1",
                                                     "/org/freedesktop/hostname1",
                                                     "org.freedesktop.hostname1",
                                                     NULL,
                                                     &error);
    if (hostnamed_proxy == NULL)
    {
        g_debug ("Couldn't get hostnamed to start, bailing: %s", error->message);
        return NULL;
    }

    vendor_variant = g_dbus_proxy_get_cached_property (hostnamed_proxy, "HardwareVendor");
    if (!vendor_variant)
    {
        g_debug ("Unable to retrieve org.freedesktop.hostname1.HardwareVendor property");
        return NULL;
    }

    model_variant = g_dbus_proxy_get_cached_property (hostnamed_proxy, "HardwareModel");
    if (!model_variant)
    {
        g_debug ("Unable to retrieve org.freedesktop.hostname1.HardwareModel property");
        return NULL;
    }

    vendor_string = g_variant_get_string (vendor_variant, NULL),
    model_string = g_variant_get_string (model_variant, NULL);

    if (vendor_string && g_strcmp0 (vendor_string, "") != 0)
    {
        gchar *vendor_model = NULL;

        vendor_model = g_strdup_printf ("%s %s", vendor_string, model_string);
        return vendor_model;
    }

    return NULL;
}

static char *
get_cpu_info (void)
{
    g_autoptr(GHashTable) counts = NULL;
    const glibtop_sysinfo *info;
    g_autoptr(GString) cpu = NULL;
    GHashTableIter iter;
    gpointer       key, value;
    int            i;
    int            j;

    counts = g_hash_table_new (g_str_hash, g_str_equal);
    info = glibtop_get_sysinfo ();

    /* count duplicates */
    for (i = 0; i != info->ncpu; ++i)
    {
        const char * const keys[] = { "model name", "cpu", "Processor" ,"Model Name"};
        char *model;
        int  *count; 
        model = NULL;

        for (j = 0; model == NULL && j != G_N_ELEMENTS (keys); ++j)
        {
             model = g_hash_table_lookup (info->cpuinfo[i].values,
                                          keys[j]);
        }

        if (model == NULL)
            continue;
        count = g_hash_table_lookup (counts, model);
        if (count == NULL)
            g_hash_table_insert (counts, model, GINT_TO_POINTER (1));
        else
            g_hash_table_replace (counts, model, GINT_TO_POINTER (GPOINTER_TO_INT (count) + 1));
    }

    cpu = g_string_new (NULL);
    g_hash_table_iter_init (&iter, counts);
    while (g_hash_table_iter_next (&iter, &key, &value))
    {
        g_autofree char *cleanedup = NULL;
        int count;

        count = GPOINTER_TO_INT (value);
        cleanedup = info_cleanup ((const char *) key);
        if (count > 1)
            g_string_append_printf (cpu, "%s \303\227 %d ", cleanedup, count);
        else
            g_string_append_printf (cpu, "%s ", cleanedup);
    }

    return g_strdup (cpu->str);
}

static char *
get_renderer_from_session (void)
{
    g_autoptr(GDBusProxy) session_proxy = NULL;
    g_autoptr(GVariant) renderer_variant = NULL;
    char *renderer;
    g_autoptr(GError) error = NULL;

    session_proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                   G_DBUS_PROXY_FLAGS_NONE,
                                                   NULL,
                                                   "org.gnome.SessionManager",
                                                   "/org/gnome/SessionManager",
                                                   "org.gnome.SessionManager",
                                                   NULL, &error);
    if (error != NULL)
    {
        g_warning ("Unable to connect to create a proxy for org.gnome.SessionManager: %s",
                   error->message);
        return NULL;
    }

    renderer_variant = g_dbus_proxy_get_cached_property (session_proxy, "Renderer");

    if (!renderer_variant)
    {
        g_warning ("Unable to retrieve org.gnome.SessionManager.Renderer property");
        return NULL;
    }

    renderer = info_cleanup (g_variant_get_string (renderer_variant, NULL));

    return renderer;
}

static gchar *
get_graphics_hardware_string (void)
{
    g_autofree char *discrete_renderer = NULL;
    g_autofree char *renderer = NULL;

    renderer = get_renderer_from_session ();
    if (!renderer)
        return g_strdup ("Unknown");

    return g_strdup (renderer);
}

static char *
get_primary_disk_size (void)
{
    g_autoptr(UDisksClient) client = NULL;
    GDBusObjectManager *manager;
    g_autolist(GDBusObject) objects = NULL;
    GList *l;
    gchar *size;
    guint64 total_size;
    g_autoptr(GError) error = NULL;

    total_size = 0;

    client = udisks_client_new_sync (NULL, &error);
    if (client == NULL)
    {
        g_warning ("Unable to get UDisks client: %s. Disk information will not be available.",
                   error->message);
        return g_strdup ("Unknown");
    }

    manager = udisks_client_get_object_manager (client);
    objects = g_dbus_object_manager_get_objects (manager);

    for (l = objects; l != NULL; l = l->next)
    {
        UDisksDrive *drive;
        drive = udisks_object_peek_drive (UDISKS_OBJECT (l->data));

        /* Skip removable devices */
        if (drive == NULL ||
          udisks_drive_get_removable (drive) ||
          udisks_drive_get_ejectable (drive))
        {
            continue;
        }

        total_size += udisks_drive_get_size (drive);
    }
    if (total_size > 0)
    {
        size = g_format_size (total_size);
    }
    else
    {
        size = g_strdup ("Unknown");
    }

    return size;
}

static char *
get_os_name (void)
{
    g_autofree gchar *name = NULL;
    g_autofree gchar *version_id = NULL;
    g_autofree gchar *pretty_name = NULL;
    g_autofree gchar *name_version = NULL;
    gchar *result = NULL;

    name = g_get_os_info (G_OS_INFO_KEY_NAME);
    version_id = g_get_os_info (G_OS_INFO_KEY_VERSION_ID);
    pretty_name = g_get_os_info (G_OS_INFO_KEY_PRETTY_NAME);

    if (pretty_name)
        name_version = g_strdup (pretty_name);
    else if (name && version_id)
        name_version = g_strdup_printf ("%s %s", name, version_id);
    else
        name_version = g_strdup (("Unknown"));

    result = g_strdup (name_version);

    return result;
}

static char *
get_os_type (void)
{
    if (GLIB_SIZEOF_VOID_P == 8)
        /* translators: This is the type of architecture for the OS */
        return g_strdup_printf ("64-bit");
    else
        /* translators: This is the type of architecture for the OS */
        return g_strdup_printf ("32-bit");
}

static char *
get_windowing_system (void)
{
    GdkDisplay *display;

    display = gdk_display_get_default ();

#if defined(GDK_WINDOWING_X11)
    if (GDK_IS_X11_DISPLAY (display))
        return g_strdup ("X11");
#endif /* GDK_WINDOWING_X11 */
#if defined(GDK_WINDOWING_WAYLAND)
    if (GDK_IS_WAYLAND_DISPLAY (display))
        return g_strdup ("Wayland");
#endif /* GDK_WINDOWING_WAYLAND */
    return g_strdup (C_("Windowing system (Wayland, X11, or Unknown)", "Unknown"));
}

static char *
get_kernel_vesrion (void)
{
    struct utsname un;

    if (uname (&un) < 0)
        return NULL;

    return g_strdup_printf ("%s %s", un.sysname, un.release);
}

static struct {
    const char *id;
    const char *display;
}   const virt_tech[] = {
    { "kvm", "KVM" },
    { "qemu", "QEmu" },
    { "vmware", "VMware" },
    { "microsoft", "Microsoft" },
    { "oracle", "Oracle" },
    { "xen", "Xen" },
    { "bochs", "Bochs" },
    { "chroot", "chroot" },
    { "openvz", "OpenVZ" },
    { "lxc", "LXC" },
    { "lxc-libvirt", "LXC (libvirt)" },
    { "systemd-nspawn", "systemd (nspawn)" }
};

static char *
get_virtualization_label (const char *virt)
{
    const char *display_name;
    guint i;

    if (virt == NULL || *virt == '\0')
    {
        return NULL;
    }

    display_name = NULL;
    for (i = 0; i < G_N_ELEMENTS (virt_tech); i++)
    {
        if (g_str_equal (virt_tech[i].id, virt))
        {
            display_name = virt_tech[i].display;
            break;
        }
    }

    return display_name ? g_strdup (display_name) : g_strdup (virt);
}

static char *
get_system_virt (void)
{
    g_autoptr(GError) error = NULL;
    g_autoptr(GDBusProxy) systemd_proxy = NULL;
    g_autoptr(GVariant) variant = NULL;
    GVariant *inner;

    systemd_proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                                                   G_DBUS_PROXY_FLAGS_NONE,
                                                   NULL,
                                                   "org.freedesktop.systemd1",
                                                   "/org/freedesktop/systemd1",
                                                   "org.freedesktop.systemd1",
                                                   NULL,
                                                   &error);

    if (systemd_proxy == NULL)
    {
        g_debug ("systemd not available, bailing: %s", error->message);
        return NULL;
    }

    variant = g_dbus_proxy_call_sync (systemd_proxy,
                                      "org.freedesktop.DBus.Properties.Get",
                                      g_variant_new ("(ss)", "org.freedesktop.systemd1.Manager", "Virtualization"),
                                      G_DBUS_CALL_FLAGS_NONE,
                                      -1,
                                      NULL,
                                      &error);
    if (variant == NULL)
    {
        g_debug ("Failed to get property '%s': %s", "Virtualization", error->message);
        return NULL;
    }

    g_variant_get (variant, "(v)", &inner);

    return get_virtualization_label (g_variant_get_string (inner, NULL));
}

static char *
get_mate_desktop_version ()
{
    int   status;
    int   i = 0;
    char *version = NULL;
    char *argv[] = {"/usr/bin/mate-about", "-v", NULL};
    g_autoptr(GError) error = NULL;

    g_debug ("About to launch '%s'", argv[0]);

    if (!g_spawn_sync (NULL, (char **) argv, NULL, 0, NULL, NULL, &version, NULL, &status, &error))
    {
        g_warning ("Failed to get GPU: %s", error->message);
        return NULL;
    }
#if GLIB_CHECK_VERSION(2, 70, 0)
    if (!g_spawn_check_wait_status (status, NULL))
#else
    if (!g_spawn_check_exit_status (status, NULL))
#endif
        return NULL;

    while (version[i] != '\0')
    {
        if (version[i] == '\n')
        {
            version[i] = '\0';
            break;
        }
        i++;
    }

    return version;
}

static char *
get_logo_name (void)
{
    char *logo_name = NULL;

    logo_name = g_get_os_info ("LOGO");

    if (logo_name == NULL)
        logo_name = g_strdup ("mate-desktop");

    return logo_name;
}

void
mate_system_info_setup (MateSystemInfo *info)
{
    g_autofree char *logo_name = NULL;
    g_autofree char *hostname_text = NULL;
    g_autofree char *hw_model_text = NULL;
    g_autofree char *memory_text = NULL;
    g_autofree char *cpu_text = NULL;
    g_autofree char *os_type_text = NULL;
    g_autofree char *os_name_text = NULL;
    g_autofree char *disk_text = NULL;
    g_autofree char *kernel_text = NULL;
    g_autofree char *windowing_system_text = NULL;
    g_autofree char *virt_text = NULL;
    g_autofree char *de_text = NULL;
    g_autofree char *graphics_hardware_string = NULL;

    GtkWidget  *label;
    glibtop_mem mem;

    logo_name = get_logo_name ();
    gtk_image_set_from_icon_name (GTK_IMAGE (info->logo_image), logo_name, GTK_ICON_SIZE_INVALID);
    gtk_image_set_pixel_size (GTK_IMAGE (info->logo_image), 128);

    hostname_text = get_system_hostname ();
    label = g_object_get_data (G_OBJECT (info->hostname_row), "labelvalue");
    set_lable_style (label, "gray", 12, hostname_text, FALSE);

    hw_model_text = get_hardware_model ();
    if (hw_model_text != NULL)
    {
        gtk_widget_show_all (info->hardware_model_row);
        label = g_object_get_data (G_OBJECT (info->hardware_model_row), "labelvalue");
        set_lable_style (label, "gray", 12, hw_model_text, FALSE);
    }

    glibtop_get_mem (&mem);
    memory_text = g_format_size_full (mem.total, G_FORMAT_SIZE_IEC_UNITS);
    label = g_object_get_data (G_OBJECT (info->memory_row), "labelvalue");
    set_lable_style (label, "gray", 12, memory_text, FALSE);

    cpu_text = get_cpu_info ();
    label = g_object_get_data (G_OBJECT (info->processor_row), "labelvalue");
    set_lable_style (label, "gray", 12, cpu_text, FALSE);

    graphics_hardware_string = get_graphics_hardware_string ();
    label = g_object_get_data (G_OBJECT (info->graphics_row), "labelvalue");
    set_lable_style (label, "gray", 12, graphics_hardware_string, FALSE);

    disk_text = get_primary_disk_size ();
    label = g_object_get_data (G_OBJECT (info->disk_row), "labelvalue");
    set_lable_style (label, "gray", 12, disk_text, FALSE);

    kernel_text = get_kernel_vesrion ();
    label = g_object_get_data (G_OBJECT (info->kernel_row), "labelvalue");
    set_lable_style (label, "gray", 12, kernel_text, FALSE);

    virt_text = get_system_virt ();
    if (virt_text != NULL)
    {
        gtk_widget_show_all (info->virtualization_row);
        label = g_object_get_data (G_OBJECT (info->virtualization_row), "labelvalue");
        set_lable_style (label, "gray", 12, virt_text, FALSE);
    }
    windowing_system_text = get_windowing_system ();
    label = g_object_get_data (G_OBJECT (info->windowing_system_row), "labelvalue");
    set_lable_style (label, "gray", 12, windowing_system_text, FALSE);

    de_text = get_mate_desktop_version ();
    label = g_object_get_data (G_OBJECT (info->mate_version_row), "labelvalue");
    set_lable_style (label, "gray", 12, de_text, FALSE);

    os_type_text = get_os_type ();
    label = g_object_get_data (G_OBJECT (info->os_type_row), "labelvalue");
    set_lable_style (label, "gray", 12, os_type_text, FALSE);

    os_name_text = get_os_name ();
    label = g_object_get_data (G_OBJECT (info->os_name_row), "labelvalue");
    set_lable_style (label, "gray", 12, os_name_text, FALSE);
}

static void
mate_system_info_destroy (GtkWidget *widget)
{
    GTK_WIDGET_CLASS (mate_system_info_parent_class)->destroy (widget);
}

static void
mate_system_info_class_init (MateSystemInfoClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    widget_class->destroy = mate_system_info_destroy;
    gtk_widget_class_set_template_from_resource (widget_class, "/org/mate/control-center/system-info/mate-system-info.ui");

    gtk_widget_class_bind_template_child (widget_class, MateSystemInfo, hostname_row);
    gtk_widget_class_bind_template_child (widget_class, MateSystemInfo, hardware_box);
    gtk_widget_class_bind_template_child (widget_class, MateSystemInfo, disk_row);
    gtk_widget_class_bind_template_child (widget_class, MateSystemInfo, mate_version_row);
    gtk_widget_class_bind_template_child (widget_class, MateSystemInfo, graphics_row);
    gtk_widget_class_bind_template_child (widget_class, MateSystemInfo, hardware_model_row);
    gtk_widget_class_bind_template_child (widget_class, MateSystemInfo, memory_row);
    gtk_widget_class_bind_template_child (widget_class, MateSystemInfo, os_box);
    gtk_widget_class_bind_template_child (widget_class, MateSystemInfo, logo_image);
    gtk_widget_class_bind_template_child (widget_class, MateSystemInfo, os_name_row);
    gtk_widget_class_bind_template_child (widget_class, MateSystemInfo, os_type_row);
    gtk_widget_class_bind_template_child (widget_class, MateSystemInfo, processor_row);
    gtk_widget_class_bind_template_child (widget_class, MateSystemInfo, virtualization_row);
    gtk_widget_class_bind_template_child (widget_class, MateSystemInfo, kernel_row);
    gtk_widget_class_bind_template_child (widget_class, MateSystemInfo, windowing_system_row);
}

static void
mate_system_info_init (MateSystemInfo *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));
    g_resources_register (mate_system_info_get_resource ());
    mate_system_info_set_row (MATE_SYSTEM_INFO (self));
}

GtkWidget *
mate_system_info_new (void)
{
    GtkWidget  *dialog;
    gboolean    use_header;
    GdkDisplay *display;

    g_object_get (gtk_settings_get_default (),
                 "gtk-dialogs-use-header", &use_header,
                  NULL);

    display = gdk_display_get_default ();
#if defined(GDK_WINDOWING_WAYLAND)
    if (GDK_IS_WAYLAND_DISPLAY (display))
        use_header = FALSE;
#endif /* GDK_WINDOWING_WAYLAND */

    dialog = g_object_new (MATE_TYPE_SYSTEM_INFO,
                           "use-header-bar", use_header,
                           NULL);
    gtk_window_set_title (GTK_WINDOW (dialog), _("Mate System Info"));
    gtk_widget_set_size_request (GTK_WIDGET (dialog), 600, 500);
    gtk_window_set_resizable  (GTK_WINDOW (dialog), FALSE);

    return dialog;
}
