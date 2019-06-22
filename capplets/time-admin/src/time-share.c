/*  time-admin
*   Copyright (C) 2018  zhuyaliang https://github.com/zhuyaliang/
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.

*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.

*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "time-share.h"
#include <glib/gi18n.h>

/******************************************************************************
* Function:            MessageReport
*
* Explain: Prompt information dialog
*
* Input:  @Title           Message title
*         @Msg             Message content
*         @nType           Message type
* Output:
*
* Author:  zhuyaliang  25/05/2018
******************************************************************************/
int MessageReport(const char *Title,const char *Msg,int nType)
{
    GtkWidget *dialog = NULL;
    int nRet;

    switch(nType)
    {
        case ERROR:
        {
            dialog = gtk_message_dialog_new(GTK_WINDOW(WindowLogin),
                                            GTK_DIALOG_DESTROY_WITH_PARENT,
                                            GTK_MESSAGE_ERROR,
                                            GTK_BUTTONS_OK,
                                            "%s",Title);
            break;
        }
        case WARING:
        {
            dialog = gtk_message_dialog_new(GTK_WINDOW(WindowLogin),
                                            GTK_DIALOG_DESTROY_WITH_PARENT,
                                            GTK_MESSAGE_WARNING,
                                            GTK_BUTTONS_OK,
                                            "%s",Title);
            break;
        }
        case INFOR:
        {
            dialog = gtk_message_dialog_new(GTK_WINDOW(WindowLogin),
                                            GTK_DIALOG_DESTROY_WITH_PARENT,
                                            GTK_MESSAGE_INFO,
                                            GTK_BUTTONS_OK,
                                            "%s",Title);
            break;
        }
        case QUESTION:
        {
            dialog = gtk_message_dialog_new(GTK_WINDOW(WindowLogin),
                                            GTK_DIALOG_DESTROY_WITH_PARENT,
                                            GTK_MESSAGE_QUESTION,
                                            GTK_BUTTONS_YES_NO,
                                            "%s",Title);
            gtk_dialog_add_button (GTK_DIALOG (dialog),("_Return"),
                                   GTK_RESPONSE_ACCEPT);
            break;
        }
        case QUESTIONNORMAL:
        {
            dialog = gtk_message_dialog_new(GTK_WINDOW(WindowLogin),
                                            GTK_DIALOG_DESTROY_WITH_PARENT,
                                            GTK_MESSAGE_QUESTION,
                                            GTK_BUTTONS_YES_NO,
                                            "%s",Title);
            break;
        }
        default :
            break;

    }
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG(dialog),
                                              "%s", Msg);
    nRet =  gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    return nRet;
}

void QuitApp(TimeAdmin *ta)
{
    if(ta->UpdateTimeId > 0)
    {
        g_source_remove (ta->UpdateTimeId);
    }
    if(ta->ApplyId > 0)
    {
        g_source_remove(ta->ApplyId);
    }

    gtk_main_quit();
}

void SetTooltip(GtkWidget *box, gboolean mode)
{
    gchar *text = mode ? NULL : _("Network time synchronization has been set up. Manual time/date setting is disabled.");
    gtk_widget_set_tooltip_text (box, text);
}
