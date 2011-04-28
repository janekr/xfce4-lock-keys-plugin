plugindir = $(libexecdir)/xfce4/panel-plugins

plugin_LTLIBRARIES = \
	liblock-keys.la

liblock_keys_la_SOURCES = \
	lock-keys.c

liblock_keys_la_CFLAGS = \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(LIBXFCE4UI_CFLAGS) \
	$(LIBXFCE4PANEL_CFLAGS) \
	$(PLATFORM_CFLAGS) \
	-DDATA_DIR=\"$(datadir)/xfce4/xfce4-lock-keys-plugin\" \
	-DG_LOG_DOMAIN=\"xfce4-lock-keys-plugin\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"

liblock_keys_la_LDFLAGS = \
	-avoid-version \
	-module

liblock_keys_la_LIBADD = \
	$(LIBXFCE4UTIL_LIBS) \
	$(LIBXFCE4UI_LIBS) \
	$(LIBXFCE4PANEL_LIBS)

#
# Desktop file
#
desktopdir =								\
	$(datadir)/xfce4/panel-plugins

desktop_in_in_files =							\
	lock-keys.desktop.in.in

desktop_in_files =							\
	$(desktop_in_in_files:.desktop.in.in=.desktop.in)

%.desktop.in: %.desktop.in.in
	sed -e "s,\@libexecdir\@,$(libexecdir),g" < $< > $@

desktop_DATA =								\
	$(desktop_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST =								\
	$(desktop_in_in_files)

CLEANFILES =								\
	$(desktop_in_files)						\
	$(desktop_DATA)
