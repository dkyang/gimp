/* small gui test app for the new unit entry widget developed during Google Summer of Code 2011 */

#include "config.h"

#include <string.h>
#include <glib-object.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include "libgimpbase/gimpbase.h"

#include "devel-docs/tools/units.h"

#include "gimpunitentries.h"

/* global objects */
GtkWidget *window;
GtkWidget *vbox;
GtkWidget *valign;

GimpUnitEntries *entries;

/* set up interface */
void
create_interface(void)
{
  GimpUnitEntry *a, *b;

  /* main window */
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_container_set_border_width (GTK_CONTAINER (window), 10);
  gtk_window_set_position (GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  gtk_window_set_default_size (GTK_WINDOW(window), 200, 100);

  /* vbox (used for the entries) */
  vbox = gtk_vbox_new (FALSE, 1);

  /* valign */
  valign = gtk_alignment_new (0, 0, 1, 0);
  gtk_container_add (GTK_CONTAINER (valign), vbox);
  gtk_container_add (GTK_CONTAINER (window), valign);

  /* entry table */
  entries = GIMP_UNIT_ENTRIES (gimp_unit_entries_new ());
  gimp_unit_entries_add_entry (entries, "width", "Width");
  gimp_unit_entries_add_entry (entries, "height", "Height");
  gimp_unit_entries_add_preview_label (entries, GIMP_UNIT_PIXEL, "width", "height");

  gimp_unit_entries_add_chain_button (entries, "width", "height");

  /* set some default values */
  a = gimp_unit_entries_get_entry (entries, "width");
  b = gimp_unit_entries_get_entry (entries, "height");
  gimp_unit_adjustment_set_value (gimp_unit_entry_get_adjustment (a), 20);
  gimp_unit_adjustment_set_value (gimp_unit_entry_get_adjustment (b), 20);
  gimp_unit_entries_set_resolution (entries,
                                    "width",  100.0,
                                    "height", 100.0,
                                    NULL);
  gimp_unit_entries_set_bounds (entries, GIMP_UNIT_PIXEL,
                                "width", 0.0, 1000.0,
                                "height", 0.0, 2000.0,
                                NULL);

  gtk_box_pack_start (GTK_BOX (vbox), gimp_unit_entries_get_table (entries), FALSE, TRUE, 0);

  /* resolution entry */
  entries = GIMP_UNIT_ENTRIES (gimp_unit_entries_new ());
  gimp_unit_entries_add_entry (entries, "xres", "X Resolution");
  gimp_unit_entries_add_entry (entries, "yres", "Y Resolution");

  /* set some default values */
  a = gimp_unit_entries_get_entry (entries, "xres");
  b = gimp_unit_entries_get_entry (entries, "yres");
  gimp_unit_adjustment_set_value (gimp_unit_entry_get_adjustment (a), 72);
  gimp_unit_adjustment_set_value (gimp_unit_entry_get_adjustment (b), 72);
  gimp_unit_entry_set_mode (a, GIMP_UNIT_ENTRY_MODE_RESOLUTION);
  gimp_unit_entry_set_mode (b, GIMP_UNIT_ENTRY_MODE_RESOLUTION);

  gtk_box_pack_end (GTK_BOX (vbox), gimp_unit_entries_get_table (entries), FALSE, TRUE, 5);

  /* signals */
  g_signal_connect_swapped (G_OBJECT(window), "destroy",
                            G_CALLBACK(gtk_main_quit), NULL);
 
  gtk_widget_show_all (window);
}

int main (int   argc,
          char *argv[])
{
    units_init();

    gtk_init (&argc, &argv);

    create_interface ();

    gtk_main ();
    
    return 0;
}