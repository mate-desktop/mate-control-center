/* mate-about-me-fingerprint.h
 * Copyright (C) 2008 Bastien Nocera <hadess@hadess.net>
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

#ifndef __MATE_ABOUT_ME_FINGERPRINT_H__
#define __MATE_ABOUT_ME_FINGERPRINT_H__

#include <gtk/gtk.h>

void set_fingerprint_label (GtkWidget *enable,
			    GtkWidget *disable);
void fingerprint_button_clicked (GtkBuilder *dialog,
				 GtkWidget *enable,
				 GtkWidget *disable);

#endif /* __MATE_ABOUT_ME_FINGERPRINT_H__ */
