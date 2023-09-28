/* 
 * Copyright (C) 2023 MATE Developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <gtk/gtk.h>
#include <fcntl.h>
#include <sys/types.h>
#include <libintl.h>
#include <locale.h>
#include <glib/gi18n.h>

#ifndef __MATE_SYSTEM_INFO_H__
#define __MATE_SYSTEM_INFO_H__  1

G_BEGIN_DECLS

#define MATE_TYPE_SYSTEM_INFO (mate_system_info_get_type ())
G_DECLARE_FINAL_TYPE (MateSystemInfo, mate_system_info, MATE, SYSTEM_INFO, GtkDialog)

GtkWidget *mate_system_info_new   (void);

void       mate_system_info_setup (MateSystemInfo *info);

G_END_DECLS

#endif
