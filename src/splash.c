/*
 * Adpated from GIMP's app/gui/splash.c
 * Copyright (C) 2002 Anthony Wong
 */

/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#if HAVE_GTK

#include <gtk/gtk.h>
#include <X11/X.h>
#include <X11/Xatom.h>

#include "splash.h"


static GtkWidget *win_initstatus = NULL;
static GtkWidget *label1         = NULL;
static GtkWidget *label2         = NULL;
static GtkWidget *progress       = NULL;


void
splash_create (gchar* filename, int duration)
{
  GtkWidget *vbox;
  GdkPixbuf *pixbuf = NULL;
  GdkAtom atom;

  win_initstatus = gtk_window_new (GTK_WINDOW_TOPLEVEL);
#if 0
  /* Not needed in these WMs: enlightenment, sawfish, metacity, fvwm2, wmaker */
  gtk_window_set_type_hint (GTK_WINDOW (win_initstatus),
                            GDK_WINDOW_TYPE_HINT_DIALOG);
#endif

  gtk_window_set_title (GTK_WINDOW (win_initstatus), filename);
  gtk_window_set_wmclass (GTK_WINDOW (win_initstatus), "hotkeys_splash", "Hotkeys");
  gtk_window_set_position (GTK_WINDOW (win_initstatus), GTK_WIN_POS_CENTER);
  gtk_window_set_resizable (GTK_WINDOW (win_initstatus), FALSE);
  gtk_window_set_modal (GTK_WINDOW (win_initstatus), TRUE);
  gtk_window_set_policy(GTK_WINDOW(win_initstatus), FALSE, FALSE, TRUE);
  gtk_widget_realize(win_initstatus);
  gdk_window_set_decorations(win_initstatus->window, 0);
#if 0
  /* Not needed in these WMs: enlightenment, sawfish, metacity, fvwm2, wmaker */
  atom = gdk_atom_intern ("_NET_WM_WINDOW_TYPE_SPLASH", FALSE);
  gdk_property_change (win_initstatus->window,
          gdk_atom_intern ("_NET_WM_WINDOW_TYPE", FALSE),
          (GdkAtom)XA_ATOM, 32, GDK_PROP_MODE_REPLACE,
          (unsigned char *)&atom, 1);
#endif

  g_signal_connect (G_OBJECT (win_initstatus), "delete_event",
                    G_CALLBACK (gtk_true), NULL);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (win_initstatus), vbox);
  gtk_widget_show (vbox);

  pixbuf = gdk_pixbuf_new_from_file (filename, NULL);
      
  if (pixbuf)
  {
      GtkWidget *align;
      GtkWidget *image;

      image = gtk_image_new_from_pixbuf (pixbuf);
      g_object_unref (pixbuf);

      align = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
      gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, TRUE, 0);
      gtk_widget_show (align);

      gtk_container_add (GTK_CONTAINER (align), image);
      gtk_widget_show (image);
  }
  else
  {
      g_message("Cannot open image");
      return;
  }

/*
  label1 = gtk_label_new ("");
  gtk_box_pack_start_defaults (GTK_BOX (vbox), label1);
  gtk_widget_show (label1);

  label2 = gtk_label_new ("");
  gtk_box_pack_start_defaults (GTK_BOX (vbox), label2);
  gtk_widget_show (label2);

  progress = gtk_progress_bar_new ();
  gtk_box_pack_start_defaults (GTK_BOX (vbox), progress);
  gtk_widget_show (progress);
*/

  gtk_widget_show (win_initstatus);
  gtk_timeout_add( duration, splash_destroy, NULL);
}

gint
splash_destroy ( gpointer data )
{
  if (win_initstatus)
    {
      gtk_widget_destroy (win_initstatus);
      win_initstatus = NULL;
    }
  exit(0);
}

#if 0
void
splash_update (const gchar *text1,
	       const gchar *text2,
	       gdouble      percentage)
{
  if (!win_initstatus)
    return;

  if (text1)
    gtk_label_set_text (GTK_LABEL (label1), text1);

  if (text2)
    gtk_label_set_text (GTK_LABEL (label2), text2);
  
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress), 
                                 CLAMP (percentage, 0.0, 1.0));
  
  while (gtk_events_pending ())
    gtk_main_iteration ();
}
#endif

#endif /* HAVE_GTK */
