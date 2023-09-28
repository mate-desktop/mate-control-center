/*************************************************************************
 File Name: main.c
 Copyright (C) 2023  MATE Developers
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <https://www.gnu.org/licenses/>.

************************************************************************/
#include "mate-system-info.h"
#include "capplet-util.h"

int main (int argc, char *argv[])
{
    GtkWidget *dialog;

    capplet_init (NULL, &argc, &argv);

    dialog = mate_system_info_new ();
    mate_system_info_setup (MATE_SYSTEM_INFO (dialog));
    g_signal_connect (dialog, "response", G_CALLBACK (gtk_main_quit), NULL);
    gtk_widget_show (dialog);

    gtk_main ();

    return 0;
}
