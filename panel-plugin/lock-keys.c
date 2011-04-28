/*
 *  Copyright (c) 2011 Jan Rękorajski <baggins@pld-linux.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <X11/XKBlib.h>
#include <gtk/gtk.h>
#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/libxfce4panel.h>

enum {
	CAPSLOCK = 0,
	NUMLOCK,
	SCROLLLOCK
} Leds;

#define IMG_CAPS_ON	DATA_DIR "/" "key-caps-on.png"
#define IMG_CAPS_OFF	DATA_DIR "/" "key-caps-off.png"
#define IMG_NUM_ON	DATA_DIR "/" "key-num-on.png"
#define IMG_NUM_OFF	DATA_DIR "/" "key-num-off.png"
#define IMG_SCROLL_ON	DATA_DIR "/" "key-scroll-on.png"
#define IMG_SCROLL_OFF	DATA_DIR "/" "key-scroll-off.png"

typedef struct LockKeysPlugin
{
    XfcePanelPlugin *plugin;

    /* plugin data */
    Display *rootwin;
    int xkbev, xkberr;
    gboolean on[3];
    gchar *tooltip;
    GdkPixbuf *pix_on[3], *pix_off[3];
    GtkWidget *pix[3];

    /* panel widgets */
    GtkWidget       *ebox;
    GtkWidget       *hbox;
    GtkWidget       *vbox;

    /* lockkeys settings */
    gboolean show[3];
} LockKeysPlugin;

static void lockkeys_construct(XfcePanelPlugin *plugin);

/* define the plugin */
XFCE_PANEL_PLUGIN_REGISTER_INTERNAL(lockkeys_construct);

void lockkeys_save(XfcePanelPlugin *plugin, LockKeysPlugin *lockkeys)
{
	XfceRc *rc;
	gchar *file;

	/* get the config file location */
	file = xfce_panel_plugin_save_location(plugin, TRUE);

	if (G_UNLIKELY(file == NULL)) {
		DBG("Failed to open config file");
		return;
	}

	/* open the config file, read/write */
	rc = xfce_rc_simple_open(file, FALSE);
	g_free(file);

	if (G_LIKELY(rc != NULL)) {
		/* save the settings */
		DBG(".");
		xfce_rc_write_bool_entry(rc, "show_capslock", lockkeys->show[CAPSLOCK]);
		xfce_rc_write_bool_entry(rc, "show_numlock", lockkeys->show[NUMLOCK]);
		xfce_rc_write_bool_entry(rc, "show_scrolllock", lockkeys->show[SCROLLLOCK]);

		/* close the rc file */
		xfce_rc_close(rc);
	}
}

static void lockkeys_read(LockKeysPlugin *lockkeys)
{
	XfceRc *rc;
	gchar *file;
	const gchar *value;

	lockkeys->show[CAPSLOCK] = TRUE;
	lockkeys->show[NUMLOCK] = TRUE;
	lockkeys->show[SCROLLLOCK] = TRUE;

	/* get the plugin config file location */
	file = xfce_panel_plugin_save_location(lockkeys->plugin, TRUE);

	if (G_LIKELY(file != NULL)) {
		/* open the config file, readonly */
		rc = xfce_rc_simple_open(file, TRUE);

		/* cleanup */
		g_free(file);

		if (G_LIKELY(rc != NULL)) {
			/* read the settings */
			lockkeys->show[CAPSLOCK] = xfce_rc_read_bool_entry(rc, "show_capslock", TRUE);
			lockkeys->show[NUMLOCK] = xfce_rc_read_bool_entry(rc, "show_numlock", TRUE);
			lockkeys->show[SCROLLLOCK] = xfce_rc_read_bool_entry(rc, "show_scrolllock", TRUE);

			/* cleanup */
			xfce_rc_close(rc);

			/* leave the function, everything went well */
			return;
		}
	}
}

static void
lockkeys_reorder_icons(int size, int orient, LockKeysPlugin *lockkeys)
{
	GtkBox *box;
	int count = 0;

	g_assert(lockkeys);

	/* how many buttons do we have?
	 * if TRUE == 1 then we could do this easier,
	 * but who knows...
	 */
	count += lockkeys->show[CAPSLOCK] ? 1 : 0;
	count += lockkeys->show[NUMLOCK] ? 1 : 0;
	count += lockkeys->show[SCROLLLOCK] ? 1 : 0;

	if (count <= 0) {
		lockkeys->show[CAPSLOCK] = TRUE;
		lockkeys->show[NUMLOCK] = TRUE;
		lockkeys->show[SCROLLLOCK] = TRUE;
		count = 3;
	}

	/* Extract the pixmaps out of the boxes, in case they 
	 * have been put there before...
	 * they have to be ref'ed before doing so
	 */
	if (lockkeys->pix[CAPSLOCK]->parent) {
		gtk_widget_ref(lockkeys->pix[CAPSLOCK]);
		gtk_container_remove(GTK_CONTAINER(lockkeys->pix[CAPSLOCK]->parent), lockkeys->pix[CAPSLOCK]);
	}
	if (lockkeys->pix[NUMLOCK]->parent) {
		gtk_widget_ref(lockkeys->pix[NUMLOCK]);
		gtk_container_remove(GTK_CONTAINER(lockkeys->pix[NUMLOCK]->parent), lockkeys->pix[NUMLOCK]);
	}
	if (lockkeys->pix[SCROLLLOCK]->parent) {
		gtk_widget_ref(lockkeys->pix[SCROLLLOCK]);
		gtk_container_remove(GTK_CONTAINER(lockkeys->pix[SCROLLLOCK]->parent), lockkeys->pix[SCROLLLOCK]);
	}

	/* Do we put the pixmaps into the vbox or the hbox?
	 * this depends on size of the panel and how many pixmaps are shown
	 */
	if (orient == GTK_ORIENTATION_HORIZONTAL)
		box = GTK_BOX(size < 32 ? lockkeys->hbox : lockkeys->vbox);
	else
		box = GTK_BOX(size < 32 ? lockkeys->vbox : lockkeys->hbox);

	if ((count < 3) || (size < 32) || (size >= 48)) {
		if (lockkeys->show[CAPSLOCK])
			gtk_box_pack_start(box, lockkeys->pix[CAPSLOCK], TRUE, TRUE, 0);
		if (lockkeys->show[NUMLOCK])
			gtk_box_pack_start(box, lockkeys->pix[NUMLOCK], TRUE, TRUE, 0);
		if (lockkeys->show[SCROLLLOCK])
			gtk_box_pack_start(box, lockkeys->pix[SCROLLLOCK], TRUE, TRUE, 0);
	} else {
		gtk_box_pack_start(GTK_BOX(lockkeys->vbox), lockkeys->pix[CAPSLOCK], TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(lockkeys->hbox), lockkeys->pix[NUMLOCK], TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(lockkeys->hbox), lockkeys->pix[SCROLLLOCK], TRUE, TRUE, 0);
	}

	gtk_widget_show_all(GTK_WIDGET(lockkeys->ebox));
}

static void
lockkeys_configure_response (GtkWidget    *dialog,
                           gint          response,
                           LockKeysPlugin *lockkeys)
{
  gboolean result;

      /* remove the dialog data from the plugin */
      g_object_set_data (G_OBJECT (lockkeys->plugin), "dialog", NULL);

      /* unlock the panel menu */
      xfce_panel_plugin_unblock_menu (lockkeys->plugin);

      /* save the plugin */
      lockkeys_save (lockkeys->plugin, lockkeys);

      /* destroy the properties dialog */
      gtk_widget_destroy (dialog);

      lockkeys_reorder_icons(xfce_panel_plugin_get_size(lockkeys->plugin),
		      xfce_panel_plugin_get_orientation(lockkeys->plugin), lockkeys);
}

static void
lockkeys_configure_cb(GtkToggleButton *toggle, LockKeysPlugin *lockkeys)
{
	gboolean *opt;

	opt = g_object_get_data(G_OBJECT(toggle), "cfg_opt");
	g_assert(opt != NULL);
	*opt = gtk_toggle_button_get_active(toggle);
}

void
lockkeys_configure (XfcePanelPlugin *plugin,
                  LockKeysPlugin    *lockkeys)
{
  GtkWidget *dialog;
  GtkWidget *tmp_widget;
  GtkWidget *vbox_menu;

  /* block the plugin menu */
  xfce_panel_plugin_block_menu (plugin);

  /* create the dialog */
  dialog = xfce_titled_dialog_new_with_buttons (_("Lock Keys Plugin"),
                                                GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
                                                GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
                                                GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                                                NULL);

  /* center dialog on the screen */
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);

  /* set dialog icon */
  gtk_window_set_icon_name (GTK_WINDOW (dialog), "xfce4-settings");

  /* link the dialog to the plugin, so we can destroy it when the plugin
   * is closed, but the dialog is still open */
  g_object_set_data (G_OBJECT (plugin), "dialog", dialog);

  /* connect the reponse signal to the dialog */
  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK(lockkeys_configure_response), lockkeys);

  vbox_menu = gtk_vbox_new(FALSE, 4);
  gtk_widget_show(vbox_menu);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), vbox_menu, FALSE, FALSE, 0);

  tmp_widget = gtk_check_button_new_with_mnemonic(_("Show _caps-lock"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp_widget), lockkeys->show[CAPSLOCK]);
  g_object_set_data(G_OBJECT(tmp_widget), "cfg_opt", &(lockkeys->show[CAPSLOCK]));
  g_signal_connect(G_OBJECT(tmp_widget), "toggled",
		   G_CALLBACK(lockkeys_configure_cb), lockkeys);
  gtk_widget_show(tmp_widget);
  gtk_box_pack_start(GTK_BOX(vbox_menu), tmp_widget, FALSE, FALSE, 0);

  tmp_widget = gtk_check_button_new_with_mnemonic(_("Show _num-lock"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp_widget), lockkeys->show[NUMLOCK]);
  g_object_set_data(G_OBJECT(tmp_widget), "cfg_opt", &(lockkeys->show[NUMLOCK]));
  g_signal_connect(G_OBJECT(tmp_widget), "toggled",
		   G_CALLBACK(lockkeys_configure_cb), lockkeys);
  gtk_widget_show(tmp_widget);
  gtk_box_pack_start(GTK_BOX(vbox_menu), tmp_widget, FALSE, FALSE, 0);

  tmp_widget = gtk_check_button_new_with_mnemonic(_("Show _scroll-lock"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp_widget), lockkeys->show[SCROLLLOCK]);
  g_object_set_data(G_OBJECT(tmp_widget), "cfg_opt", &(lockkeys->show[SCROLLLOCK]));
  g_signal_connect(G_OBJECT(tmp_widget), "toggled",
		   G_CALLBACK(lockkeys_configure_cb), lockkeys);
  gtk_widget_show(tmp_widget);
  gtk_box_pack_start(GTK_BOX(vbox_menu), tmp_widget, FALSE, FALSE, 0);

  /* show the entire dialog */
  gtk_widget_show (dialog);
}

static gboolean init_xkb_extension(LockKeysPlugin *lockkeys)
{
	int code;
	int maj = XkbMajorVersion;
	int min = XkbMinorVersion;

	if (!XkbLibraryVersion(&maj, &min))
		return FALSE;
	if (!XkbQueryExtension(lockkeys->rootwin, &code, &lockkeys->xkbev, &lockkeys->xkberr, &maj, &min))
		return FALSE;
	return TRUE;
}

static void change_icons(LockKeysPlugin *lockkeys)
{
	if (lockkeys->on[CAPSLOCK])
		gtk_image_set_from_pixbuf(GTK_IMAGE(lockkeys->pix[CAPSLOCK]), lockkeys->pix_on[CAPSLOCK]);
	else
		gtk_image_set_from_pixbuf(GTK_IMAGE(lockkeys->pix[CAPSLOCK]), lockkeys->pix_off[CAPSLOCK]);

	if (lockkeys->on[NUMLOCK])
		gtk_image_set_from_pixbuf(GTK_IMAGE(lockkeys->pix[NUMLOCK]), lockkeys->pix_on[NUMLOCK]);
	else
		gtk_image_set_from_pixbuf(GTK_IMAGE(lockkeys->pix[NUMLOCK]), lockkeys->pix_off[NUMLOCK]);

	if (lockkeys->on[SCROLLLOCK])
		gtk_image_set_from_pixbuf(GTK_IMAGE(lockkeys->pix[SCROLLLOCK]), lockkeys->pix_on[SCROLLLOCK]);
	else
		gtk_image_set_from_pixbuf(GTK_IMAGE(lockkeys->pix[SCROLLLOCK]), lockkeys->pix_off[SCROLLLOCK]);
}

void ledstates_changed(LockKeysPlugin *lockkeys, unsigned int state)
{
	int i;
	char *on, *off;

	for (i = 0; i < 3; i++) {
		if (state & (1 << i))
			lockkeys->on[i] = 1;
		else
			lockkeys->on[i] = 0;
	}

	change_icons(lockkeys);

	on = _("On");
	off = _("Off");
	g_free(lockkeys->tooltip);
	lockkeys->tooltip = g_strdup_printf(_("Caps: %s Num: %s Scroll: %s"),
					  lockkeys->on[CAPSLOCK] ? on : off,
					  lockkeys->on[NUMLOCK] ? on : off,
					  lockkeys->on[SCROLLLOCK] ? on :
					  off);

	gtk_widget_set_tooltip_text(GTK_WIDGET(lockkeys->plugin), lockkeys->tooltip);
}

GdkFilterReturn event_filter(GdkXEvent *gdkxevent, GdkEvent *event,
			     LockKeysPlugin *lockkeys)
{
	XkbEvent ev;
	memcpy(&ev.core, gdkxevent, sizeof(ev.core));

	if (ev.core.type == lockkeys->xkbev + XkbEventCode) {
		if (ev.any.xkb_type == XkbIndicatorStateNotify)
			ledstates_changed(lockkeys, ev.indicators.state);
	}

	return GDK_FILTER_CONTINUE;
}

static LockKeysPlugin *lockkeys_new(XfcePanelPlugin *plugin)
{
	LockKeysPlugin *lockkeys;
	GdkDrawable *drawable;
	GError *error;

	/* allocate memory for the plugin structure */
	lockkeys = panel_slice_new0(LockKeysPlugin);

	/* pointer to plugin */
	lockkeys->plugin = plugin;

	lockkeys->tooltip = NULL;

	if ((lockkeys->pix_on[CAPSLOCK] =
	     gdk_pixbuf_new_from_file(IMG_CAPS_ON, &error)) == NULL) {
		xfce_dialog_show_error(NULL, NULL, _("Problem loading pixmaps."));
	}
	if ((lockkeys->pix_on[NUMLOCK] =
	     gdk_pixbuf_new_from_file(IMG_NUM_ON, &error)) == NULL) {
		xfce_dialog_show_error(NULL, NULL, _("Problem loading pixmaps."));
	}
	if ((lockkeys->pix_on[SCROLLLOCK] =
	     gdk_pixbuf_new_from_file(IMG_SCROLL_ON, &error)) == NULL) {
		xfce_dialog_show_error(NULL, NULL, _("Problem loading pixmaps."));
	}
	if ((lockkeys->pix_off[CAPSLOCK] =
	     gdk_pixbuf_new_from_file(IMG_CAPS_OFF, &error)) == NULL) {
		xfce_dialog_show_error(NULL, NULL, _("Problem loading pixmaps."));
	}
	if ((lockkeys->pix_off[NUMLOCK] =
	     gdk_pixbuf_new_from_file(IMG_NUM_OFF, &error)) == NULL) {
		xfce_dialog_show_error(NULL, NULL, _("Problem loading pixmaps."));
	}
	if ((lockkeys->pix_off[SCROLLLOCK] =
	     gdk_pixbuf_new_from_file(IMG_SCROLL_OFF, &error)) == NULL) {
		xfce_dialog_show_error(NULL, NULL, _("Problem loading pixmaps."));
	}

	lockkeys->pix[CAPSLOCK] = gtk_image_new();
	lockkeys->pix[NUMLOCK] = gtk_image_new();
	lockkeys->pix[SCROLLLOCK] = gtk_image_new();
	change_icons(lockkeys);

	/* read the user settings */
	lockkeys_read(lockkeys);

	drawable = gdk_get_default_root_window();
	g_assert(drawable);
	lockkeys->rootwin = gdk_x11_drawable_get_xdisplay(drawable);

	/* create some panel widgets */
	lockkeys->ebox = gtk_event_box_new();
	gtk_widget_show(lockkeys->ebox);

	lockkeys->vbox = gtk_vbox_new(FALSE, 0);
	lockkeys->hbox = gtk_hbox_new(FALSE, 0);

	gtk_container_add(GTK_CONTAINER(lockkeys->ebox), lockkeys->vbox);

	gtk_box_pack_end(GTK_BOX(lockkeys->vbox), lockkeys->hbox, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(lockkeys->hbox), lockkeys->pix[CAPSLOCK], TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(lockkeys->hbox), lockkeys->pix[NUMLOCK], TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(lockkeys->hbox), lockkeys->pix[SCROLLLOCK], TRUE, TRUE, 0);

	lockkeys_reorder_icons(xfce_panel_plugin_get_size(lockkeys->plugin),
			     xfce_panel_plugin_get_orientation
			     (lockkeys->plugin), lockkeys);

	gtk_widget_show_all(lockkeys->ebox);

	if (!init_xkb_extension(lockkeys))
		xfce_dialog_show_error(NULL, NULL, _("Could not initialize X Keyboard Extension."));
	else {
		unsigned int state = 0;

		XkbGetIndicatorState(lockkeys->rootwin, XkbUseCoreKbd, &state);
		ledstates_changed(lockkeys, state);
		if (XkbSelectEvents(lockkeys->rootwin, XkbUseCoreKbd, XkbIndicatorStateNotifyMask, XkbIndicatorStateNotifyMask))
			gdk_window_add_filter(NULL, (GdkFilterFunc) event_filter, lockkeys);
	}

	return lockkeys;
}

static void lockkeys_free(XfcePanelPlugin *plugin, LockKeysPlugin *lockkeys)
{
	GtkWidget *dialog;

	/* check if the dialog is still open. if so, destroy it */
	dialog = g_object_get_data(G_OBJECT(plugin), "dialog");
	if (G_UNLIKELY(dialog != NULL))
		gtk_widget_destroy(dialog);

	/* destroy the panel widgets */
	gtk_widget_destroy(lockkeys->hbox);
	gtk_widget_destroy(lockkeys->vbox);

	g_free(lockkeys->tooltip);

	/* free the plugin structure */
	panel_slice_free(LockKeysPlugin, lockkeys);
}

static void lockkeys_orientation_changed(XfcePanelPlugin *plugin,
				       GtkOrientation orientation,
				       LockKeysPlugin *lockkeys)
{
	lockkeys_reorder_icons(xfce_panel_plugin_get_size(plugin), orientation, lockkeys);
}

static gboolean lockkeys_size_changed(XfcePanelPlugin *plugin,
				    gint size, LockKeysPlugin *lockkeys)
{
	lockkeys_reorder_icons(size, xfce_panel_plugin_get_orientation(plugin), lockkeys);
	return TRUE;
}

static void lockkeys_construct(XfcePanelPlugin *plugin)
{
	LockKeysPlugin *lockkeys;

	/* setup transation domain */
	xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

	/* create the plugin */
	lockkeys = lockkeys_new(plugin);

	/* add the ebox to the panel */
	gtk_container_add(GTK_CONTAINER(plugin), lockkeys->ebox);

	/* show the panel's right-click menu on this ebox */
	xfce_panel_plugin_add_action_widget(plugin, lockkeys->ebox);

	/* connect plugin signals */
	g_signal_connect(G_OBJECT(plugin), "free-data",
			 G_CALLBACK(lockkeys_free), lockkeys);

	g_signal_connect(G_OBJECT(plugin), "save",
			 G_CALLBACK(lockkeys_save), lockkeys);

	g_signal_connect(G_OBJECT(plugin), "size-changed",
			 G_CALLBACK(lockkeys_size_changed), lockkeys);

	g_signal_connect(G_OBJECT(plugin), "orientation-changed",
			 G_CALLBACK(lockkeys_orientation_changed), lockkeys);

	/* show the configure menu item and connect signal */
	xfce_panel_plugin_menu_show_configure(plugin);
	g_signal_connect(G_OBJECT(plugin), "configure-plugin",
			 G_CALLBACK(lockkeys_configure), lockkeys);
}
