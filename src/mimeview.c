/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2007 Hiroyuki Yamamoto and the Claws Mail team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "defs.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkctree.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvpaned.h>
#include <gtk/gtktext.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkdnd.h>
#include <gtk/gtkselection.h>
#include <gtk/gtktooltips.h>
#include <gtk/gtkcontainer.h>
#include <gtk/gtkbutton.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef HAVE_APACHE_FNMATCH
/* kludge: apache's fnmatch clashes with <regex.h>, don't include
 * fnmatch.h */
#include <fnmatch.h>
#endif

#include "main.h"
#include "mimeview.h"
#include "textview.h"
#include "procmime.h"
#include "summaryview.h"
#include "menu.h"
#include "filesel.h"
#include "alertpanel.h"
#include "inputdialog.h"
#include "utils.h"
#include "gtkutils.h"
#include "prefs_common.h"
#include "procheader.h"
#include "stock_pixmap.h"
#include "gtk/gtkvscrollbutton.h"
#include "gtk/logwindow.h"
#include "timing.h"

#ifdef MAEMO
#include <libosso.h>
#ifndef CHINOOK
#include <osso-mime.h>
#else
#include <hildon-mime.h>
#endif
#endif

typedef enum
{
	COL_MIMETYPE = 0,
	COL_SIZE     = 1,
	COL_NAME     = 2
} MimeViewColumnPos;

#define N_MIMEVIEW_COLS	3

static void mimeview_set_multipart_tree		(MimeView	*mimeview,
						 MimeInfo	*mimeinfo,
						 GtkCTreeNode	*parent);
static GtkCTreeNode *mimeview_append_part	(MimeView	*mimeview,
						 MimeInfo	*partinfo,
						 GtkCTreeNode	*parent);
static void mimeview_show_message_part		(MimeView	*mimeview,
						 MimeInfo	*partinfo);
static void mimeview_change_view_type		(MimeView	*mimeview,
						 MimeViewType	 type);
static gchar *mimeview_get_filename_for_part		(MimeInfo	*partinfo,
						 const gchar	*basedir,
						 gint		 number);
static gboolean mimeview_write_part		(const gchar	*filename,
						 MimeInfo	*partinfo);

static void mimeview_selected		(GtkCTree	*ctree,
					 GtkCTreeNode	*node,
					 gint		 column,
					 MimeView	*mimeview);
static void mimeview_start_drag 	(GtkWidget	*widget,
					 gint		 button,
					 GdkEvent	*event,
					 MimeView	*mimeview);
static gint mimeview_button_pressed	(GtkWidget	*widget,
					 GdkEventButton	*event,
					 MimeView	*mimeview);
static gint mimeview_key_pressed	(GtkWidget	*widget,
					 GdkEventKey	*event,
					 MimeView	*mimeview);

static void mimeview_drag_data_get      (GtkWidget	  *widget,
					 GdkDragContext   *drag_context,
					 GtkSelectionData *selection_data,
					 guint		   info,
					 guint		   time,
					 MimeView	  *mimeview);

static gboolean mimeview_scrolled	(GtkWidget	*widget,
					 GdkEventScroll	*event,
					 MimeView	*mimeview);
static void mimeview_display_as_text	(MimeView	*mimeview);
static void mimeview_save_as		(MimeView	*mimeview);
static void mimeview_save_all		(MimeView	*mimeview);
static void mimeview_launch		(MimeView	*mimeview,
					 MimeInfo	*partinfo);
static void mimeview_open_with		(MimeView	*mimeview);
static void mimeview_open_part_with	(MimeView	*mimeview,
					 MimeInfo	*partinfo,
					 gboolean	 automatic);
static void mimeview_select_next_part	(MimeView	*mimeview);
static void mimeview_select_prev_part	(MimeView	*mimeview);
static void mimeview_view_file		(const gchar	*filename,
					 MimeInfo	*partinfo,
					 const gchar	*cmd,
					 MimeView  	*mimeview);
static gboolean icon_clicked_cb		(GtkWidget 	*button, 
					 GdkEventButton	*event, 
					 MimeView 	*mimeview);
static void icon_selected               (MimeView       *mimeview, 
					 gint            num, 
					 MimeInfo       *partinfo);
static gint icon_key_pressed            (GtkWidget      *button, 
					 GdkEventKey    *event,
					 MimeView       *mimeview);
static void icon_list_append_icon 	(MimeView 	*mimeview, 
					 MimeInfo 	*mimeinfo);
static void icon_list_create		(MimeView 	*mimeview, 
					 MimeInfo 	*mimeinfo);
static void icon_list_clear		(MimeView	*mimeview);
static void icon_list_toggle_by_mime_info (MimeView	*mimeview,
					   MimeInfo	*mimeinfo);
static gint mime_toggle_button_cb(GtkWidget *button, GdkEventButton *event,
				    MimeView *mimeview);
static gboolean part_button_pressed	(MimeView 	*mimeview, 
					 GdkEventButton *event, 
					 MimeInfo 	*partinfo);
static void icon_scroll_size_allocate_cb(GtkWidget 	*widget, 
					 GtkAllocation  *layout_size, 
					 MimeView 	*mimeview);
static MimeInfo *mimeview_get_part_to_use(MimeView *mimeview);

static void mimeview_launch_cb(MimeView *mimeview)
{
	mimeview_launch(mimeview, mimeview_get_part_to_use(mimeview));
}
static GtkItemFactoryEntry mimeview_popup_entries[] =
{
	{N_("/_Open (l)"),		  NULL, mimeview_launch_cb,	  0, NULL},
#ifndef MAEMO
	{N_("/Open _with (o)..."),	  NULL, mimeview_open_with,	  0, NULL},
#endif
	{N_("/_Display as text (t)"), 	  NULL, mimeview_display_as_text, 0, NULL},
	{N_("/_Save as (y)..."),	  NULL, mimeview_save_as,	  0, NULL},
	{N_("/Save _all..."),	 	  NULL, mimeview_save_all,	  0, NULL},
	{N_("/Next part (a)"),	 	  NULL, mimeview_select_next_part,0, NULL},
};

static GtkTargetEntry mimeview_mime_types[] =
{
	{"text/uri-list", 0, 0}
};

GSList *mimeviewer_factories;
GSList *mimeviews;

static GdkCursor *hand_cursor = NULL;

static gboolean mimeview_visi_notify(GtkWidget *widget,
				       GdkEventVisibility *event,
				       MimeView *mimeview)
{
	gdk_window_set_cursor(widget->window, hand_cursor);
	return FALSE;
}

static gboolean mimeview_leave_notify(GtkWidget *widget,
				      GdkEventCrossing *event,
				      MimeView *mimeview)
{
	gdk_window_set_cursor(widget->window, NULL);
	return FALSE;
}

static gboolean mimeview_enter_notify(GtkWidget *widget,
				      GdkEventCrossing *event,
				      MimeView *mimeview)
{
	gdk_window_set_cursor(widget->window, hand_cursor);
	return FALSE;
}

MimeView *mimeview_create(MainWindow *mainwin)
{
	MimeView *mimeview;

	GtkWidget *paned;
	GtkWidget *scrolledwin;
	GtkWidget *ctree;
	GtkWidget *mime_notebook;
	GtkWidget *popupmenu;
	GtkWidget *ctree_mainbox;
	GtkWidget *vbox;
	GtkWidget *mime_toggle;
	GtkWidget *icon_mainbox;
	GtkWidget *icon_scroll;
	GtkWidget *icon_vbox;
	GtkWidget *arrow;
	GtkWidget *scrollbutton;
	GtkWidget *hbox;
	GtkTooltips *tooltips;
	GtkItemFactory *popupfactory;
	NoticeView *siginfoview;
	gchar *titles[N_MIMEVIEW_COLS];
	gint n_entries;
	gint i;

	if (!hand_cursor)
		hand_cursor = gdk_cursor_new(GDK_HAND2);

	debug_print("Creating MIME view...\n");
	mimeview = g_new0(MimeView, 1);

	titles[COL_MIMETYPE] = _("MIME Type");
	titles[COL_SIZE]     = _("Size");
	titles[COL_NAME]     = _("Name");

	scrolledwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scrolledwin);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwin),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);

	ctree = gtk_sctree_new_with_titles(N_MIMEVIEW_COLS, 0, titles);
	gtk_widget_show(ctree);
	gtk_clist_set_selection_mode(GTK_CLIST(ctree), GTK_SELECTION_BROWSE);
	gtk_ctree_set_line_style(GTK_CTREE(ctree), GTK_CTREE_LINES_NONE);
	gtk_clist_set_column_justification(GTK_CLIST(ctree), COL_SIZE,
					   GTK_JUSTIFY_RIGHT);
	gtk_clist_set_column_width(GTK_CLIST(ctree), COL_MIMETYPE, 240);
	gtk_clist_set_column_width(GTK_CLIST(ctree), COL_SIZE, 90);
	for (i = 0; i < N_MIMEVIEW_COLS; i++)
		GTK_WIDGET_UNSET_FLAGS(GTK_CLIST(ctree)->column[i].button,
				       GTK_CAN_FOCUS);
	gtk_container_add(GTK_CONTAINER(scrolledwin), ctree);

	g_signal_connect(G_OBJECT(ctree), "tree_select_row",
			 G_CALLBACK(mimeview_selected), mimeview);
	g_signal_connect(G_OBJECT(ctree), "button_release_event",
			 G_CALLBACK(mimeview_button_pressed), mimeview);
	g_signal_connect(G_OBJECT(ctree), "key_press_event",
			 G_CALLBACK(mimeview_key_pressed), mimeview);
	g_signal_connect(G_OBJECT (ctree),"start_drag",
			 G_CALLBACK (mimeview_start_drag), mimeview);
	g_signal_connect(G_OBJECT(ctree), "drag_data_get",
			 G_CALLBACK(mimeview_drag_data_get), mimeview);

	mime_notebook = gtk_notebook_new();
        gtk_widget_show(mime_notebook);
        GTK_WIDGET_UNSET_FLAGS(mime_notebook, GTK_CAN_FOCUS);
        gtk_notebook_set_show_tabs(GTK_NOTEBOOK(mime_notebook), FALSE);
        gtk_notebook_set_show_border(GTK_NOTEBOOK(mime_notebook), FALSE);
	
	icon_vbox = gtk_vbox_new(FALSE, 2);
	gtk_widget_show(icon_vbox);
	icon_scroll = gtk_layout_new(NULL, NULL);
	gtk_widget_show(icon_scroll);
	gtk_layout_put(GTK_LAYOUT(icon_scroll), icon_vbox, 0, 0);
	scrollbutton = gtk_vscrollbutton_new(gtk_layout_get_vadjustment(GTK_LAYOUT(icon_scroll)));
	gtk_widget_show(scrollbutton);

	g_signal_connect(G_OBJECT(icon_scroll), "scroll_event",
			 G_CALLBACK(mimeview_scrolled), mimeview);

    	mime_toggle = gtk_event_box_new();
	gtk_event_box_set_visible_window(GTK_EVENT_BOX(mime_toggle), FALSE);

	g_signal_connect(G_OBJECT(mime_toggle), "visibility-notify-event",
			 G_CALLBACK(mimeview_visi_notify), mimeview);
	g_signal_connect(G_OBJECT(mime_toggle), "motion-notify-event",
			 G_CALLBACK(mimeview_visi_notify), mimeview);
	g_signal_connect(G_OBJECT(mime_toggle), "leave-notify-event",
			 G_CALLBACK(mimeview_leave_notify), mimeview);
	g_signal_connect(G_OBJECT(mime_toggle), "enter-notify-event",
			 G_CALLBACK(mimeview_enter_notify), mimeview);

	gtk_container_set_border_width(GTK_CONTAINER(mime_toggle), 2);
	gtk_widget_show(mime_toggle);
	mimeview->ctree_mode = FALSE;
	arrow = gtk_arrow_new(GTK_ARROW_LEFT, GTK_SHADOW_NONE);
	gtk_widget_show(arrow);
	gtk_container_add(GTK_CONTAINER(mime_toggle), arrow);
	g_signal_connect(G_OBJECT(mime_toggle), "button_release_event", 
			 G_CALLBACK(mime_toggle_button_cb), mimeview);

	icon_mainbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(icon_mainbox);
	gtk_widget_set_size_request(icon_mainbox, 32, -1);
	gtk_box_pack_start(GTK_BOX(icon_mainbox), mime_toggle, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(icon_mainbox), icon_scroll, TRUE, TRUE, 3);
	gtk_box_pack_end(GTK_BOX(icon_mainbox), scrollbutton, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(icon_mainbox), "size_allocate", 
			 G_CALLBACK(icon_scroll_size_allocate_cb), mimeview);
	
	ctree_mainbox = gtk_hbox_new(FALSE, 0);	
	gtk_box_pack_start(GTK_BOX(ctree_mainbox), scrolledwin, TRUE, TRUE, 0);

 	n_entries = sizeof(mimeview_popup_entries) /
 		sizeof(mimeview_popup_entries[0]);
 	popupmenu = menu_create_items(mimeview_popup_entries, n_entries,
 				      "<MimeView>", &popupfactory, mimeview);
	tooltips = gtk_tooltips_new();

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	siginfoview = noticeview_create(mainwin);
	noticeview_hide(siginfoview);
	noticeview_set_icon_clickable(siginfoview, TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), mime_notebook, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(vbox), GTK_WIDGET_PTR(siginfoview), FALSE, FALSE, 0);

	paned = gtk_vpaned_new();
	gtk_widget_show(paned);
	gtk_paned_set_gutter_size(GTK_PANED(paned), 0);
	gtk_paned_pack1(GTK_PANED(paned), ctree_mainbox, FALSE, TRUE);
	gtk_paned_pack2(GTK_PANED(paned), vbox, TRUE, TRUE);
	
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), paned, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), icon_mainbox, FALSE, FALSE, 0);

	gtk_widget_show(hbox);
	gtk_widget_hide(ctree_mainbox);
#ifdef GENERIC_UMPC
	gtk_widget_set_size_request(mime_toggle, -1, arrow->requisition.height + 8);
#endif
	mimeview->hbox          = hbox;
	mimeview->paned         = paned;
	mimeview->scrolledwin   = scrolledwin;
	mimeview->ctree         = ctree;
	mimeview->mime_notebook = mime_notebook;
	mimeview->popupmenu     = popupmenu;
	mimeview->popupfactory  = popupfactory;
	mimeview->type          = -1;
	mimeview->ctree_mainbox = ctree_mainbox;
	mimeview->icon_scroll   = icon_scroll;
	mimeview->icon_vbox     = icon_vbox;
	mimeview->icon_mainbox  = icon_mainbox;
	mimeview->icon_count    = 0;
	mimeview->mainwin       = mainwin;
	mimeview->tooltips      = tooltips;
	mimeview->oldsize       = 60;
	mimeview->mime_toggle   = mime_toggle;
	mimeview->siginfoview	= siginfoview;
	mimeview->scrollbutton  = scrollbutton;
	mimeview->target_list	= gtk_target_list_new(mimeview_mime_types, 1); 
	
	mimeviews = g_slist_prepend(mimeviews, mimeview);

	return mimeview;
}

void mimeview_init(MimeView *mimeview)
{
	textview_init(mimeview->textview);

	gtk_container_add(GTK_CONTAINER(mimeview->mime_notebook),
		GTK_WIDGET_PTR(mimeview->textview));
}

static gboolean any_part_is_signed(MimeInfo *mimeinfo)
{
	while (mimeinfo) {
		if (privacy_mimeinfo_is_signed(mimeinfo))
			return TRUE;
		mimeinfo = procmime_mimeinfo_next(mimeinfo);
	}

	return FALSE;
}

void mimeview_show_message(MimeView *mimeview, MimeInfo *mimeinfo,
			   const gchar *file)
{
	GtkCTree *ctree = GTK_CTREE(mimeview->ctree);
	GtkCTreeNode *node;

	mimeview_clear(mimeview);

	g_return_if_fail(file != NULL);
	g_return_if_fail(mimeinfo != NULL);

	mimeview->mimeinfo = mimeinfo;

	mimeview->file = g_strdup(file);

	g_signal_handlers_block_by_func(G_OBJECT(ctree), mimeview_selected,
					mimeview);

	/* check if the mail's signed - it can change the mail structure */
	
	if (any_part_is_signed(mimeinfo))
		debug_print("signed mail\n");

	mimeview_set_multipart_tree(mimeview, mimeinfo, NULL);
	icon_list_clear(mimeview);
	icon_list_create(mimeview, mimeinfo);

	g_signal_handlers_unblock_by_func(G_OBJECT(ctree),
					  mimeview_selected, mimeview);

	node = GTK_CTREE_NODE(GTK_CLIST(ctree)->row_list);
	if (node) {
		gtk_ctree_select(ctree, node);
		icon_list_toggle_by_mime_info
			(mimeview, gtk_ctree_node_get_row_data(ctree, node));
		gtkut_ctree_set_focus_row(ctree, node);
	}
}

#ifdef USE_PTHREAD
static void mimeview_check_sig_cancel_now(MimeView *mimeview);
#endif

static void mimeview_free_mimeinfo(MimeView *mimeview)
{
	gboolean defer = FALSE;
#ifdef USE_PTHREAD
	defer = (mimeview->check_data != NULL);
	if (defer)
		mimeview->check_data->free_after_use = TRUE;
#endif
	if (mimeview->mimeinfo != NULL && !defer) {
		procmime_mimeinfo_free_all(mimeview->mimeinfo);
		mimeview->mimeinfo = NULL;
	} else if (defer) {
#ifdef USE_PTHREAD
		debug_print("deferring free(mimeinfo) and cancelling check\n");
		mimeview_check_sig_cancel_now(mimeview);
#endif
	}
}

void mimeview_destroy(MimeView *mimeview)
{
	GSList *cur;
	
	for (cur = mimeview->viewers; cur != NULL; cur = g_slist_next(cur)) {
		MimeViewer *viewer = (MimeViewer *) cur->data;
		viewer->destroy_viewer(viewer);
	}
	g_slist_free(mimeview->viewers);
	gtk_target_list_unref(mimeview->target_list);

	mimeview_free_mimeinfo(mimeview);
#ifdef USE_PTHREAD
	if (mimeview->check_data) {
		mimeview->check_data->destroy_mimeview = TRUE;
		debug_print("deferring destroy\n");
	} else 
#endif
	{
		g_free(mimeview->file);
		g_free(mimeview);
		mimeviews = g_slist_remove(mimeviews, mimeview);
	}
	
}

MimeInfo *mimeview_get_selected_part(MimeView *mimeview)
{
	return gtk_ctree_node_get_row_data
		(GTK_CTREE(mimeview->ctree), mimeview->opened);
}

gint mimeview_get_selected_part_num(MimeView *mimeview)
{
	gint i = 0;
	GList *rows = GTK_CLIST(mimeview->ctree)->row_list;
	while (rows) {
		if (mimeview->opened == GTK_CTREE_NODE(rows))
			return i;
		i++;
		rows = rows->next;
	}
	return -1;
}

void mimeview_select_part_num(MimeView *mimeview, gint i)
{
	if (i < 0)
		return;
	gtk_clist_unselect_all(GTK_CLIST(mimeview->ctree));
	gtk_clist_select_row(GTK_CLIST(mimeview->ctree), i, -1);
	gtkut_clist_set_focus_row(GTK_CLIST(mimeview->ctree), i);
}

static void mimeview_set_multipart_tree(MimeView *mimeview,
					MimeInfo *mimeinfo,
					GtkCTreeNode *parent)
{
	GtkCTreeNode *node;

	g_return_if_fail(mimeinfo != NULL);

	while (mimeinfo != NULL) {
		node = mimeview_append_part(mimeview, mimeinfo, parent);

		if (mimeinfo->node->children)
			mimeview_set_multipart_tree(mimeview, (MimeInfo *) mimeinfo->node->children->data, node);
		mimeinfo = mimeinfo->node->next != NULL ? (MimeInfo *) mimeinfo->node->next->data : NULL;
	}
}

static const gchar *get_real_part_name(MimeInfo *partinfo)
{
	const gchar *name = NULL;

	name = procmime_mimeinfo_get_parameter(partinfo, "filename");
	if (name == NULL)
		name = procmime_mimeinfo_get_parameter(partinfo, "name");

	return name;
}

static const gchar *get_part_name(MimeInfo *partinfo)
{
	const gchar *name;

	name = get_real_part_name(partinfo);
	if (name == NULL)
		name = "";

	return name;
}

static const gchar *get_part_description(MimeInfo *partinfo)
{
	if (partinfo->description)
		return partinfo->description;
	else
		return get_part_name(partinfo);
}

static GtkCTreeNode *mimeview_append_part(MimeView *mimeview,
					  MimeInfo *partinfo,
					  GtkCTreeNode *parent)
{
	GtkCTree *ctree = GTK_CTREE(mimeview->ctree);
	GtkCTreeNode *node;
	static gchar content_type[64];
	gchar *str[N_MIMEVIEW_COLS];

	if (partinfo->type != MIMETYPE_UNKNOWN && partinfo->subtype) {
		g_snprintf(content_type, 64, "%s/%s", procmime_get_media_type_str(partinfo->type), partinfo->subtype);
	} else {
		g_snprintf(content_type, 64, "UNKNOWN");
	}

	str[COL_MIMETYPE] = content_type;
	str[COL_SIZE] = to_human_readable(partinfo->length);
	if (prefs_common.attach_desc)
		str[COL_NAME] = (gchar *) get_part_description(partinfo);
	else
		str[COL_NAME] = (gchar *) get_part_name(partinfo);

	node = gtk_sctree_insert_node(ctree, parent, NULL, str, 0,
				     NULL, NULL, NULL, NULL,
				     FALSE, TRUE);
	gtk_ctree_node_set_row_data(ctree, node, partinfo);

	return node;
}

static void mimeview_show_message_part(MimeView *mimeview, MimeInfo *partinfo)
{
	FILE *fp;
	const gchar *fname;

	if (!partinfo) return;

	fname = mimeview->file;
	if (!fname) return;

	if ((fp = g_fopen(fname, "rb")) == NULL) {
		FILE_OP_ERROR(fname, "fopen");
		return;
	}

	if (fseek(fp, partinfo->offset, SEEK_SET) < 0) {
		FILE_OP_ERROR(mimeview->file, "fseek");
		fclose(fp);
		return;
	}

	mimeview_change_view_type(mimeview, MIMEVIEW_TEXT);
	textview_show_part(mimeview->textview, partinfo, fp);

	fclose(fp);
}

static MimeViewer *get_viewer_for_content_type(MimeView *mimeview, const gchar *content_type)
{
	GSList *cur;
	MimeViewerFactory *factory = NULL;
	MimeViewer *viewer = NULL;

/*
 * FNM_CASEFOLD is a GNU extension
 * if its not defined copy the string to the stack and
 * convert the copy to lower case
 */
#ifndef FNM_CASEFOLD
#define FNM_CASEFOLD 0
	Xstrdup_a(content_type, content_type, return NULL);
	g_strdown((gchar *)content_type);
#endif
	
	for (cur = mimeviewer_factories; cur != NULL; cur = g_slist_next(cur)) {
		MimeViewerFactory *curfactory = cur->data;
		gint i = 0;

		while (curfactory->content_types[i] != NULL) {
			if(!fnmatch(curfactory->content_types[i], content_type, FNM_CASEFOLD)) {
				debug_print("%s\n", curfactory->content_types[i]);
				factory = curfactory;
				break;
			}
			i++;
		}
		if (factory != NULL)
			break;
	}
	if (factory == NULL)
		return NULL;

	for (cur = mimeview->viewers; cur != NULL; cur = g_slist_next(cur)) {
		MimeViewer *curviewer = cur->data;
		
		if (curviewer->factory == factory)
			return curviewer;
	}
	viewer = factory->create_viewer();
	gtk_container_add(GTK_CONTAINER(mimeview->mime_notebook),
		GTK_WIDGET(viewer->get_widget(viewer)));
		
	mimeview->viewers = g_slist_append(mimeview->viewers, viewer);

	return viewer;
}

gboolean mimeview_has_viewer_for_content_type(MimeView *mimeview, const gchar *content_type)
{
	return (get_viewer_for_content_type(mimeview, content_type) != NULL);
}

static MimeViewer *get_viewer_for_mimeinfo(MimeView *mimeview, MimeInfo *partinfo)
{
	gchar *content_type = NULL;
	MimeViewer *viewer = NULL;

	if ((partinfo->type == MIMETYPE_APPLICATION) &&
            (!g_ascii_strcasecmp(partinfo->subtype, "octet-stream"))) {
		const gchar *filename;

		filename = procmime_mimeinfo_get_parameter(partinfo, "filename");
		if (filename == NULL)
			filename = procmime_mimeinfo_get_parameter(partinfo, "name");
		if (filename != NULL)
			content_type = procmime_get_mime_type(filename);
	} else {
		content_type = procmime_get_content_type_str(partinfo->type, partinfo->subtype);
	}

	if (content_type != NULL) {
		viewer = get_viewer_for_content_type(mimeview, content_type);
		g_free(content_type);
	}

	return viewer;
}

gboolean mimeview_show_part(MimeView *mimeview, MimeInfo *partinfo)
{
	MimeViewer *viewer;
	
	viewer = get_viewer_for_mimeinfo(mimeview, partinfo);
	if (viewer == NULL) {
		if (mimeview->mimeviewer != NULL)
			mimeview->mimeviewer->clear_viewer(mimeview->mimeviewer);
		mimeview->mimeviewer = NULL;
		return FALSE;
	}

	if (mimeview->mimeviewer != NULL)
		mimeview->mimeviewer->clear_viewer(mimeview->mimeviewer);

	if (mimeview->mimeviewer != viewer) {
		mimeview->mimeviewer = viewer;
		mimeview_change_view_type(mimeview, MIMEVIEW_VIEWER);
	}
	viewer->mimeview = mimeview;
	viewer->show_mimepart(viewer, mimeview->file, partinfo);

	return TRUE;
}

static void mimeview_change_view_type(MimeView *mimeview, MimeViewType type)
{
	TextView  *textview  = mimeview->textview;
	GtkWidget *focused = NULL;
	
	if (mainwindow_get_mainwindow())
		focused = gtkut_get_focused_child(
				GTK_CONTAINER(mainwindow_get_mainwindow()->window));

	if ((mimeview->type != MIMEVIEW_VIEWER) && 
	    (mimeview->type == type)) return;

	switch (type) {
	case MIMEVIEW_TEXT:
		gtk_notebook_set_current_page(GTK_NOTEBOOK(mimeview->mime_notebook),
			gtk_notebook_page_num(GTK_NOTEBOOK(mimeview->mime_notebook), 
			GTK_WIDGET_PTR(textview)));
		break;
	case MIMEVIEW_VIEWER:
		gtk_notebook_set_current_page(GTK_NOTEBOOK(mimeview->mime_notebook),
			gtk_notebook_page_num(GTK_NOTEBOOK(mimeview->mime_notebook), 
			GTK_WIDGET(mimeview->mimeviewer->get_widget(mimeview->mimeviewer))));
		break;
	default:
		return;
	}
	if (focused)
		gtk_widget_grab_focus(focused);
	mimeview->type = type;
}

void mimeview_clear(MimeView *mimeview)
{
	GtkCList *clist = NULL;
	
	if (!mimeview)
		return;

	if (g_slist_find(mimeviews, mimeview) == NULL)
		return;
	
	clist = GTK_CLIST(mimeview->ctree);
	
	noticeview_hide(mimeview->siginfoview);

	gtk_clist_clear(clist);
	textview_clear(mimeview->textview);
	if (mimeview->mimeviewer != NULL)
		mimeview->mimeviewer->clear_viewer(mimeview->mimeviewer);

	mimeview_free_mimeinfo(mimeview);

	mimeview->mimeinfo = NULL;

	mimeview->opened = NULL;

	g_free(mimeview->file);
	mimeview->file = NULL;

	icon_list_clear(mimeview);
	mimeview_change_view_type(mimeview, MIMEVIEW_TEXT);
}

static void check_signature_cb(GtkWidget *widget, gpointer user_data);
static void mimeview_check_signature(MimeView *mimeview);
static void display_full_info_cb(GtkWidget *widget, gpointer user_data);

static void update_signature_noticeview(MimeView *mimeview, MimeInfo *mimeinfo, 
					gboolean special, SignatureStatus code)
{
	gchar *text = NULL, *button_text = NULL;
	void  *func = NULL;
	StockPixmap icon = STOCK_PIXMAP_PRIVACY_SIGNED;
	SignatureStatus mycode = SIGNATURE_UNCHECKED;
	
	g_return_if_fail(mimeview != NULL);
	g_return_if_fail(mimeinfo != NULL);
	
	if (special)
		mycode = code;
	else 
		mycode = privacy_mimeinfo_get_sig_status(mimeinfo);

	switch (mycode) {
	case SIGNATURE_UNCHECKED:
		button_text = _("Check signature");
		func = check_signature_cb;
		icon = STOCK_PIXMAP_PRIVACY_SIGNED;
		break;
	case SIGNATURE_OK:
		button_text = _("View full information");
		func = display_full_info_cb;
		icon = STOCK_PIXMAP_PRIVACY_PASSED;
		break;
	case SIGNATURE_WARN:
		button_text = _("View full information");
		func = display_full_info_cb;
		icon = STOCK_PIXMAP_PRIVACY_WARN;
		break;
	case SIGNATURE_INVALID:
		button_text = _("View full information");
		func = display_full_info_cb;
		icon = STOCK_PIXMAP_PRIVACY_FAILED;
		break;
	case SIGNATURE_CHECK_FAILED:
		button_text = _("Check again");
		func = check_signature_cb;
		icon = STOCK_PIXMAP_PRIVACY_UNKNOWN;
	case SIGNATURE_CHECK_TIMEOUT:
		button_text = _("Check again");
		func = check_signature_cb;
		icon = STOCK_PIXMAP_PRIVACY_UNKNOWN;
	default:
		break;
	}
	if (mycode == SIGNATURE_UNCHECKED) {
		gchar *tmp = privacy_mimeinfo_sig_info_short(mimeinfo);
		text = g_strdup_printf("%s %s",
			tmp, _("Click the icon or hit 'C' to check it."));
		g_free(tmp);
	} else if (mycode != SIGNATURE_CHECK_TIMEOUT) {
		text = privacy_mimeinfo_sig_info_short(mimeinfo);
	} else if (mycode == SIGNATURE_CHECK_TIMEOUT) {
		text = g_strdup(_("Timeout checking the signature. Click the icon or hit 'C' to try again."));
	}

	noticeview_set_text(mimeview->siginfoview, text);
	gtk_label_set_selectable(GTK_LABEL(mimeview->siginfoview->text), TRUE);

	g_free(text);
	noticeview_set_button_text(mimeview->siginfoview, NULL);
	noticeview_set_button_press_callback(
		mimeview->siginfoview,
		G_CALLBACK(func),
		(gpointer) mimeview);
	noticeview_set_icon(mimeview->siginfoview, icon);
	noticeview_set_tooltip(mimeview->siginfoview, button_text);
}

#ifdef USE_PTHREAD

/* reset all thread stuff, and do the cleanups we've been left to do */
static void mimeview_check_data_reset(MimeView *mimeview)
{
	if (!mimeview->check_data)
		return;

	if (mimeview->check_data->free_after_use) {
		debug_print("freeing deferred mimeinfo\n");
		procmime_mimeinfo_free_all(mimeview->check_data->siginfo);
	}
	if (mimeview->check_data->destroy_mimeview) {
		debug_print("freeing deferred mimeview\n");
		g_free(mimeview->file);
		g_free(mimeview);
		mimeviews = g_slist_remove(mimeviews, mimeview);
	}

	g_free(mimeview->check_data);
	mimeview->check_data = NULL;
}

/* GUI update once the checker thread is done or killed */
static gboolean mimeview_check_sig_thread_cb(void *data)
{
	MimeView *mimeview = (MimeView *) data;
	MimeInfo *mimeinfo = mimeview->siginfo;

	debug_print("mimeview_check_sig_thread_cb\n");
	
	if (mimeinfo == NULL) {
		/* message changed !? */
		g_warning("no more siginfo!\n");
		goto end;
	}
	
	if (!mimeview->check_data) {
		g_warning("nothing to check\n");
		return FALSE;
	}

	if (mimeview->check_data->siginfo != mimeinfo) {
		/* message changed !? */
		g_warning("different siginfo!\n");
		goto end;
	}

	if (mimeview->check_data->destroy_mimeview ||
	    mimeview->check_data->free_after_use) {
	    	debug_print("not bothering, we're changing message\n"); 
		goto end;
	}
	
	/* update status */
	if (mimeview->check_data->timeout) 
		update_signature_noticeview(mimeview, mimeview->siginfo, 
			TRUE, SIGNATURE_CHECK_TIMEOUT);
	else
		update_signature_noticeview(mimeview, mimeview->siginfo, 
			FALSE, 0);
	icon_list_clear(mimeview);
	icon_list_create(mimeview, mimeview->mimeinfo);
	
end:
	mimeview_check_data_reset(mimeview);
	return FALSE;
}

/* sig checker thread */
static void *mimeview_check_sig_worker_thread(void *data)
{
	MimeView *mimeview = (MimeView *)data;
	MimeInfo *mimeinfo = mimeview->siginfo;
	
	debug_print("checking...\n");

	if (!mimeview->check_data)
		return NULL;

	if (mimeinfo && mimeinfo == mimeview->check_data->siginfo)
		privacy_mimeinfo_check_signature(mimeinfo);
	else {
		/* that's strange! we changed message without 
		 * getting killed. */
		g_warning("different siginfo!\n");
		mimeview_check_data_reset(mimeview);
		return NULL;
	}

	/* use g_timeout so that GUI updates is done from the
	 * correct thread */
	g_timeout_add(0,mimeview_check_sig_thread_cb,mimeview);
	
	return NULL;
}

/* killer thread - acts when the checker didn't work fast
 * enough. */
static void *mimeview_check_sig_cancel_thread(void *data)
{
	MimeView *mimeview = (MimeView *)data;
	
	if (!mimeview->check_data)
		return NULL; /* nothing to kill ! */

	/* wait for a few seconds... */
	debug_print("waiting a while\n");

	sleep(5);
	
	if (!mimeview->check_data)
		return NULL; /* nothing to kill, it's done in time :) */
	
	/* too late, go away checker thread */
	debug_print("killing checker thread\n");
	pthread_cancel(mimeview->check_data->th);
	
	/* tell upstream it was a timeout */
	mimeview->check_data->timeout = TRUE;
	/* use g_timeout so that GUI updates is done from the
	 * correct thread */
	g_timeout_add(0,mimeview_check_sig_thread_cb,mimeview);

	return NULL;
}

/* get rid of the checker thread right now - used when changing the
 * displayed message for example. */
static void mimeview_check_sig_cancel_now(MimeView *mimeview)
{
	if (!mimeview->check_data)
		return;
	debug_print("killing checker thread NOW\n");
	pthread_cancel(mimeview->check_data->th);

	/* tell upstream it was a timeout */
	mimeview->check_data->timeout = TRUE;
	mimeview_check_sig_thread_cb(mimeview);
	return;
}

/* creates a thread to check the signature, and a second one
 * to kill the first one after a timeout */
static void mimeview_check_sig_in_thread(MimeView *mimeview)
{
	pthread_t th, th2;
	pthread_attr_t detach, detach2;
	
	if (mimeview->check_data) {
		g_warning("already checking it");
		return;
	}
	
	mimeview->check_data = g_new0(SigCheckData, 1);
	mimeview->check_data->siginfo = mimeview->siginfo;
	debug_print("creating thread\n");

	/* init thread attributes and create the checker thread */
	if (pthread_attr_init(&detach) != 0 ||
	    pthread_attr_setdetachstate(&detach, PTHREAD_CREATE_DETACHED) != 0 ||
	    pthread_attr_init(&detach2) != 0 ||
	    pthread_attr_setdetachstate(&detach2, PTHREAD_CREATE_DETACHED) != 0 ||
	    pthread_create(&th, &detach, 
			mimeview_check_sig_worker_thread, 
			mimeview) != 0) {
		/* arh. We'll do it synchronously. */
		g_warning("can't create thread");
		g_free(mimeview->check_data);
		mimeview->check_data = NULL;
		return;
	} else 
		mimeview->check_data->th = th;

	/* create the killer thread */
	pthread_create(&th2, &detach2, 
			mimeview_check_sig_cancel_thread, 
			mimeview);
}
#endif

static void check_signature_cb(GtkWidget *widget, gpointer user_data)
{
	MimeView *mimeview = (MimeView *) user_data;
	MimeInfo *mimeinfo = mimeview->siginfo;
	
	if (mimeinfo == NULL)
		return;
#ifdef USE_PTHREAD
	if (mimeview->check_data)
		return;
#endif
	noticeview_set_text(mimeview->siginfoview, _("Checking signature..."));
	GTK_EVENTS_FLUSH();
#ifdef USE_PTHREAD
	/* let's do it non-blocking */
	mimeview_check_sig_in_thread(mimeview);
	if (!mimeview->check_data) /* let's check syncronously */
#endif
	{
		debug_print("checking without thread\n");
		privacy_mimeinfo_check_signature(mimeinfo);
		update_signature_noticeview(mimeview, mimeview->siginfo, FALSE, 0);
		icon_list_clear(mimeview);
		icon_list_create(mimeview, mimeview->mimeinfo);
	}
}

static void mimeview_check_signature(MimeView *mimeview)
{
	check_signature_cb(NULL, mimeview);	
}

static void redisplay_email(GtkWidget *widget, gpointer user_data)
{
	MimeView *mimeview = (MimeView *) user_data;
	GtkCTreeNode *node = mimeview->opened;
	mimeview->opened = NULL;
	mimeview_selected(GTK_CTREE(mimeview->ctree), node, 0, mimeview);
}

static void display_full_info_cb(GtkWidget *widget, gpointer user_data)
{
	MimeView *mimeview = (MimeView *) user_data;
	gchar *siginfo;

	siginfo = privacy_mimeinfo_sig_info_full(mimeview->siginfo);
	textview_set_text(mimeview->textview, siginfo);
	g_free(siginfo);
	noticeview_set_button_text(mimeview->siginfoview, NULL);
	noticeview_set_button_press_callback(
		mimeview->siginfoview,
		G_CALLBACK(redisplay_email),
		(gpointer) mimeview);
	noticeview_set_tooltip(mimeview->siginfoview, _("Go back to email"));
}

static void update_signature_info(MimeView *mimeview, MimeInfo *selected)
{
	MimeInfo *siginfo;
	MimeInfo *first_text;
	
	g_return_if_fail(mimeview != NULL);
	g_return_if_fail(selected != NULL);
	
	if (selected->type == MIMETYPE_MESSAGE 
	&&  !g_ascii_strcasecmp(selected->subtype, "rfc822")) {
		/* if the first text part is signed, check that */
		first_text = selected;
		while (first_text && first_text->type != MIMETYPE_TEXT) {
			first_text = procmime_mimeinfo_next(first_text);
		}
		if (first_text) {
			update_signature_info(mimeview, first_text);
			return;
		}	
	}

	siginfo = selected;
	while (siginfo != NULL) {
		if (privacy_mimeinfo_is_signed(siginfo))
			break;
		siginfo = procmime_mimeinfo_parent(siginfo);
	}
	mimeview->siginfo = siginfo;
	
	if (siginfo == NULL) {
		noticeview_hide(mimeview->siginfoview);
		return;
	}
	
	update_signature_noticeview(mimeview, siginfo, FALSE, 0);
	noticeview_show(mimeview->siginfoview);
}

static void mimeview_selected(GtkCTree *ctree, GtkCTreeNode *node, gint column,
			      MimeView *mimeview)
{
	MimeInfo *partinfo;
	if (mimeview->opened == node) return;
	mimeview->opened = node;
	mimeview->spec_part = NULL;
	gtk_ctree_node_moveto(ctree, node, -1, 0.5, 0);

	partinfo = gtk_ctree_node_get_row_data(ctree, node);
	if (!partinfo) return;

	/* ungrab the mouse event */
	if (GTK_WIDGET_HAS_GRAB(ctree)) {
		gtk_grab_remove(GTK_WIDGET(ctree));
		if (gdk_pointer_is_grabbed())
			gdk_pointer_ungrab(GDK_CURRENT_TIME);
	}
	
	mimeview->textview->default_text = FALSE;

	update_signature_info(mimeview, partinfo);

	if (!mimeview_show_part(mimeview, partinfo)) {
		switch (partinfo->type) {
		case MIMETYPE_TEXT:
		case MIMETYPE_MESSAGE:
		case MIMETYPE_MULTIPART:
			mimeview_show_message_part(mimeview, partinfo);
		
			break;
		default:
			mimeview->textview->default_text = TRUE;
			mimeview_change_view_type(mimeview, MIMEVIEW_TEXT);
			textview_clear(mimeview->textview);
			textview_show_mime_part(mimeview->textview, partinfo);
			break;
		}
	}
}

static void mimeview_start_drag(GtkWidget *widget, gint button,
				GdkEvent *event, MimeView *mimeview)
{
	GdkDragContext *context;
	MimeInfo *partinfo;

	g_return_if_fail(mimeview != NULL);

	partinfo = mimeview_get_selected_part(mimeview);
	if (partinfo->disposition == DISPOSITIONTYPE_INLINE) return;

	context = gtk_drag_begin(widget, mimeview->target_list,
				 GDK_ACTION_COPY, button, event);
	gtk_drag_set_icon_default(context);
}

static gint mimeview_button_pressed(GtkWidget *widget, GdkEventButton *event,
				    MimeView *mimeview)
{
	GtkCList *clist = GTK_CLIST(widget);
	gint row, column;

	if (!event) return FALSE;

	if (event->button == 2 || event->button == 3) {
		if (!gtk_clist_get_selection_info(clist, event->x, event->y,
						  &row, &column))
			return FALSE;
		gtk_clist_unselect_all(clist);
		gtk_clist_select_row(clist, row, column);
		gtkut_clist_set_focus_row(clist, row);
	}
	part_button_pressed(mimeview, event, mimeview_get_selected_part(mimeview));

	return FALSE;
}

static gboolean mimeview_scrolled(GtkWidget *widget, GdkEventScroll *event,
				    MimeView *mimeview)
{
	GtkVScrollbutton *scrollbutton = (GtkVScrollbutton *)mimeview->scrollbutton;
	if (event->direction == GDK_SCROLL_UP) {
		scrollbutton->scroll_type = GTK_SCROLL_STEP_BACKWARD;
	} else {
		scrollbutton->scroll_type = GTK_SCROLL_STEP_FORWARD;
	}
	gtk_vscrollbutton_scroll(scrollbutton);
	return TRUE;
}

/* from gdkevents.c */
#define DOUBLE_CLICK_TIME 250

static gboolean part_button_pressed(MimeView *mimeview, GdkEventButton *event, 
				    MimeInfo *partinfo)
{
	static MimeInfo *lastinfo;
	static guint32 lasttime;

	if (event->button == 2 ||
	    (event->button == 1 && (event->time - lasttime) < DOUBLE_CLICK_TIME && lastinfo == partinfo)) {
		/* call external program for image, audio or html */
		mimeview_launch(mimeview, partinfo);
		return TRUE;
	} else if (event->button == 3) {
		if (partinfo && (partinfo->type == MIMETYPE_MESSAGE ||
				 partinfo->type == MIMETYPE_IMAGE ||
				 partinfo->type == MIMETYPE_MULTIPART))
			menu_set_sensitive(mimeview->popupfactory,
					   "/Display as text (t)", FALSE);
		else
			menu_set_sensitive(mimeview->popupfactory,
					   "/Display as text (t)", TRUE);
		if (partinfo &&
		    partinfo->type == MIMETYPE_APPLICATION &&
		    !g_ascii_strcasecmp(partinfo->subtype, "octet-stream"))
			menu_set_sensitive(mimeview->popupfactory,
					   "/Open (l)", FALSE);
		else
			menu_set_sensitive(mimeview->popupfactory,
					   "/Open (l)", TRUE);

		g_object_set_data(G_OBJECT(mimeview->popupmenu),
				  "pop_partinfo", partinfo);
				    
		gtk_menu_popup(GTK_MENU(mimeview->popupmenu),
			       NULL, NULL, NULL, NULL,
			       event->button, event->time);
		return TRUE;
	}

	lastinfo = partinfo;
	lasttime = event->time;
	return FALSE;
}


void mimeview_pass_key_press_event(MimeView *mimeview, GdkEventKey *event)
{
	mimeview_key_pressed(mimeview->ctree, event, mimeview);
}

static void mimeview_select_next_part(MimeView *mimeview)
{
	GtkCTree *ctree = GTK_CTREE(mimeview->ctree);
	GtkCTreeNode *node = mimeview->opened;
	MimeInfo *partinfo = NULL;
	
skip:
	node = GTK_CTREE_NODE_NEXT(node);
	if (!node)
		node = gtk_ctree_node_nth(GTK_CTREE(ctree), 0);

	if (node) {
		partinfo = gtk_ctree_node_get_row_data(ctree, node);
		if (partinfo->type == MIMETYPE_MULTIPART)
			goto skip;
		gtk_sctree_unselect_all(GTK_SCTREE(ctree));
		gtk_sctree_select(GTK_SCTREE(ctree), node);
		icon_list_toggle_by_mime_info(mimeview, partinfo);
	}
}

static void mimeview_select_prev_part(MimeView *mimeview)
{
	GtkCTree *ctree = GTK_CTREE(mimeview->ctree);
	GtkCTreeNode *node = mimeview->opened;
	MimeInfo *partinfo = NULL;
	
skip:
	node = GTK_CTREE_NODE_PREV(node);
	if (!node)
		node = gtk_ctree_node_nth(GTK_CTREE(ctree), GTK_CLIST(ctree)->rows - 1);

	if (node) {
		partinfo = gtk_ctree_node_get_row_data(ctree, node);
		if (partinfo->type == MIMETYPE_MULTIPART)
			goto skip;
		gtk_sctree_unselect_all(GTK_SCTREE(ctree));
		gtk_sctree_select(GTK_SCTREE(ctree), node);
		icon_list_toggle_by_mime_info(mimeview, partinfo);
	}
}

#define BREAK_ON_MODIFIER_KEY() \
	if ((event->state & (GDK_MOD1_MASK|GDK_CONTROL_MASK)) != 0) break

#define KEY_PRESS_EVENT_STOP() \
        g_signal_stop_emission_by_name(G_OBJECT(ctree), \
                                       "key_press_event");

static gint mimeview_key_pressed(GtkWidget *widget, GdkEventKey *event,
				 MimeView *mimeview)
{
	SummaryView *summaryview;
	GtkCTree *ctree = GTK_CTREE(widget);

	if (!event) return FALSE;
	if (!mimeview->opened) return FALSE;

	summaryview = mimeview->messageview->mainwin->summaryview;
	
	if (summaryview && quicksearch_has_focus(summaryview->quicksearch))
		return FALSE;
		
	switch (event->keyval) {
	case GDK_space:
		if (mimeview_scroll_page(mimeview, FALSE))
			return TRUE;

		mimeview_select_next_part(mimeview);
		return TRUE;

	case GDK_BackSpace:
		mimeview_scroll_page(mimeview, TRUE);
		return TRUE;
	case GDK_Return:
	case GDK_KP_Enter:
		mimeview_scroll_one_line(mimeview,
					 (event->state & GDK_MOD1_MASK) != 0);
		return TRUE;
	case GDK_n:
	case GDK_N:
		BREAK_ON_MODIFIER_KEY();
		mimeview_select_next_part(mimeview);
		return TRUE;

	case GDK_p:
	case GDK_P:
		BREAK_ON_MODIFIER_KEY();
		mimeview_select_prev_part(mimeview);
		return TRUE;

	case GDK_y:
		BREAK_ON_MODIFIER_KEY();
		KEY_PRESS_EVENT_STOP();
		mimeview_save_as(mimeview);
		return TRUE;
	case GDK_t:
		BREAK_ON_MODIFIER_KEY();
		KEY_PRESS_EVENT_STOP();
		mimeview_display_as_text(mimeview);
		return TRUE;	
	case GDK_l:
		BREAK_ON_MODIFIER_KEY();
		KEY_PRESS_EVENT_STOP();
		mimeview_launch(mimeview, NULL);
		return TRUE;
	case GDK_o:
		BREAK_ON_MODIFIER_KEY();
		KEY_PRESS_EVENT_STOP();
		mimeview_open_with(mimeview);
		return TRUE;
	case GDK_c:
		BREAK_ON_MODIFIER_KEY();
		KEY_PRESS_EVENT_STOP();
		mimeview_check_signature(mimeview);
		return TRUE;
	case GDK_a:
		BREAK_ON_MODIFIER_KEY();
		KEY_PRESS_EVENT_STOP();
		mimeview_select_next_part(mimeview);
		return TRUE;
	default:
		break;
	}

	if (!mimeview->messageview->mainwin) return FALSE;

	summary_pass_key_press_event(summaryview, event);
	return TRUE;
}

static void mimeview_drag_data_get(GtkWidget	    *widget,
				   GdkDragContext   *drag_context,
				   GtkSelectionData *selection_data,
				   guint	     info,
				   guint	     time,
				   MimeView	    *mimeview)
{
	gchar *filename = NULL, *uriname, *tmp;
	MimeInfo *partinfo;
	gint err;
	gint count = 0;

	if (!mimeview->opened) return;
	if (!mimeview->file) return;

	partinfo = mimeview_get_selected_part(mimeview);
	if (!partinfo) return;

	if (strlen(get_part_name(partinfo)) > 0) {
		filename = g_path_get_basename(get_part_name(partinfo));
		if (*filename == '\0') return;
	} else if (partinfo->type == MIMETYPE_MESSAGE 
		   && !g_ascii_strcasecmp(partinfo->subtype, "rfc822")) {
		gchar *name = NULL;
		GPtrArray *headers = NULL;
		FILE *fp;

		fp = g_fopen(partinfo->data.filename, "rb");
		fseek(fp, partinfo->offset, SEEK_SET);
		headers = procheader_get_header_array_asis(fp);
		if (headers) {
			gint i;
			for (i = 0; i < headers->len; i++) {
				Header *header = g_ptr_array_index(headers, i);
				if (procheader_headername_equal(header->name, "Subject")) {
					unfold_line(header->body);
					name = g_strconcat(header->body, ".txt", NULL);
					subst_for_filename(name);
				}
			}
			procheader_header_array_destroy(headers);
		}
		fclose(fp);
		if (name)
			filename = g_path_get_basename(name);
		g_free(name);
	}
	if (filename == NULL)
		filename = g_path_get_basename("Unnamed part");

	if (!g_utf8_validate(filename, -1, NULL))
		tmp = conv_codeset_strdup(filename,
				conv_get_locale_charset_str(),
				CS_UTF_8);
	else
		tmp = g_strdup(filename);

	if (tmp == NULL) {
		g_warning("filename not in UTF-8");
		tmp = g_strdup("Unnamed part");
	}
	filename = g_strconcat(get_mime_tmp_dir(), G_DIR_SEPARATOR_S,
			       tmp, NULL);

check_new_file:
	if (is_file_exist(filename)) {
		gchar *ext = NULL;
		gchar *prefix = NULL;
		gchar *new_name = NULL;
		if (strrchr(tmp, '.')) {
			prefix = g_strdup(tmp);
			ext = g_strdup(strrchr(tmp, '.'));
			*(strrchr(prefix, '.')) = '\0';
		} else {
			prefix = g_strdup(tmp);
			ext = g_strdup("");
		}
		count++;
		new_name = g_strdup_printf("%s.%d%s", prefix, count, ext);
		g_free(prefix);
		g_free(ext);
		g_free(filename);
		filename = g_strconcat(get_mime_tmp_dir(), G_DIR_SEPARATOR_S,
			       new_name, NULL);
		g_free(new_name);
		goto check_new_file;
	}

	g_free(tmp);

	if ((err = procmime_get_part(filename, partinfo)) < 0)
		alertpanel_error
			(_("Couldn't save the part of multipart message: %s"), 
				strerror(-err));

	tmp = g_filename_to_uri(filename, NULL, NULL);
	uriname = g_strconcat(tmp, "\r\n", NULL);
	g_free(tmp);

	gtk_selection_data_set(selection_data, selection_data->target, 8,
			       (guchar *)uriname, strlen(uriname));

	g_free(uriname);
	g_free(filename);
}

/**
 * Returns a filename (with path) for an attachment
 * \param partinfo The attachment to save
 * \param basedir The target directory
 * \param number Used for dummy filename if attachment is unnamed
 */
static gchar *mimeview_get_filename_for_part(MimeInfo *partinfo,
				      const gchar *basedir,
				      gint number)
{
	gchar *fullname;
	gchar *filename;

	filename = g_strdup(get_part_name(partinfo));
	if (!filename || !*filename)
		filename = g_strdup_printf("noname.%d", number);

	if (!g_utf8_validate(filename, -1, NULL)) {
		gchar *tmp = conv_filename_to_utf8(filename);
		g_free(filename);
		filename = tmp;
	}
	
	subst_for_filename(filename);

	fullname = g_strconcat
		(basedir, G_DIR_SEPARATOR_S, (filename[0] == G_DIR_SEPARATOR)
		 ? &filename[1] : filename, NULL);

	g_free(filename);
	filename = conv_filename_from_utf8(fullname);
	g_free(fullname);
	return filename;
}

/**
 * Write a single attachment to file
 * \param filename Filename with path
 * \param partinfo Attachment to save
 */
static gboolean mimeview_write_part(const gchar *filename,
				    MimeInfo *partinfo)
{
	gchar *dir;
	gint err;

	dir= g_path_get_dirname(filename);
	if (!is_dir_exist(dir))
		make_dir_hier(dir);
	g_free(dir);

	if (is_file_exist(filename)) {
		AlertValue aval;
		gchar *res;
		gchar *tmp;
		
		if (!g_utf8_validate(filename, -1, NULL))
			tmp = conv_filename_to_utf8(filename);
		else 
			tmp = g_strdup(filename);
		
		res = g_strdup_printf(_("Overwrite existing file '%s'?"),
				      tmp);
		g_free(tmp);
		aval = alertpanel(_("Overwrite"), res, GTK_STOCK_CANCEL, 
				  GTK_STOCK_OK, NULL);
		g_free(res);					  
		if (G_ALERTALTERNATE != aval) return FALSE;
	}

	if ((err = procmime_get_part(filename, partinfo)) < 0) {
		alertpanel_error
			(_("Couldn't save the part of multipart message: %s"), 
				strerror(-err));
		return FALSE;
	}

	return TRUE;
}

/**
 * Menu callback: Save all attached files
 * \param mimeview Current display
 */
static void mimeview_save_all(MimeView *mimeview)
{
	MimeInfo *partinfo;
	gchar *dirname;
	gchar *startdir = NULL;
	gint number = 1;

	if (!mimeview->opened) return;
	if (!mimeview->file) return;
	if (!mimeview->mimeinfo) return;

	partinfo = mimeview->mimeinfo;
	if (prefs_common.attach_save_dir && *prefs_common.attach_save_dir)
		startdir = g_strconcat(prefs_common.attach_save_dir,
				       G_DIR_SEPARATOR_S, NULL);
	else
		startdir = g_strdup(get_home_dir());

	dirname = filesel_select_file_save_folder(_("Select destination folder"), startdir);
	if (!dirname) {
		g_free(startdir);
		return;
	}

	if (!is_dir_exist (dirname)) {
		alertpanel_error(_("'%s' is not a directory."),
				 dirname);
		g_free(startdir);
		return;
	}

	if (dirname[strlen(dirname)-1] == G_DIR_SEPARATOR)
		dirname[strlen(dirname)-1] = '\0';

	/* Skip the first part, that is sometimes DISPOSITIONTYPE_UNKNOWN */
	if (partinfo && partinfo->type == MIMETYPE_MESSAGE)
		partinfo = procmime_mimeinfo_next(partinfo);
	if (partinfo && partinfo->type == MIMETYPE_MULTIPART) {
		partinfo = procmime_mimeinfo_next(partinfo);
		if (partinfo && partinfo->type == MIMETYPE_TEXT)
			partinfo = procmime_mimeinfo_next(partinfo);
	}
		
	while (partinfo != NULL) {
		if (partinfo->type != MIMETYPE_MESSAGE &&
		    partinfo->type != MIMETYPE_MULTIPART &&
		    (partinfo->disposition != DISPOSITIONTYPE_INLINE
		     || get_real_part_name(partinfo) != NULL)) {
			gchar *filename = mimeview_get_filename_for_part
				(partinfo, dirname, number++);

			mimeview_write_part(filename, partinfo);
			g_free(filename);
		}
		partinfo = procmime_mimeinfo_next(partinfo);
	}

	g_free(prefs_common.attach_save_dir);
	g_free(startdir);
	prefs_common.attach_save_dir = g_filename_to_utf8(dirname,
					-1, NULL, NULL, NULL);
}

static MimeInfo *mimeview_get_part_to_use(MimeView *mimeview)
{
	MimeInfo *partinfo = NULL;
	if (mimeview->spec_part) {
		partinfo = mimeview->spec_part;
		mimeview->spec_part = NULL;
	} else {
		partinfo = (MimeInfo *) g_object_get_data
			 (G_OBJECT(mimeview->popupmenu),
			 "pop_partinfo");
		g_object_set_data(G_OBJECT(mimeview->popupmenu),
				  "pop_partinfo", NULL);
		if (!partinfo) { 
			partinfo = mimeview_get_selected_part(mimeview);
		}			 
	}

	return partinfo;
}
/**
 * Menu callback: Save the selected attachment
 * \param mimeview Current display
 */
static void mimeview_save_as(MimeView *mimeview)
{
	gchar *filename;
	gchar *filepath = NULL;
	gchar *filedir = NULL;
	MimeInfo *partinfo;
	gchar *partname = NULL;

	if (!mimeview->opened) return;
	if (!mimeview->file) return;

	partinfo = mimeview_get_part_to_use(mimeview);

	g_return_if_fail(partinfo != NULL);
	
	if (get_part_name(partinfo) == NULL) {
		return;
	}
	partname = g_strdup(get_part_name(partinfo));
	
	if (!g_utf8_validate(partname, -1, NULL)) {
		gchar *tmp = conv_filename_to_utf8(partname);
		if (!tmp) {
			tmp = conv_codeset_strdup(partname,
				conv_get_locale_charset_str(),
				CS_UTF_8);
		}
		if (tmp) {
			g_free(partname);
			partname = tmp;
		}
	}

	subst_for_filename(partname);
	
	if (prefs_common.attach_save_dir)
		filepath = g_strconcat(prefs_common.attach_save_dir,
				       G_DIR_SEPARATOR_S, partname, NULL);
	else
		filepath = g_strdup(partname);

	g_free(partname);

	filename = filesel_select_file_save(_("Save as"), filepath);
	if (!filename) {
		g_free(filepath);
		return;
	}

	mimeview_write_part(filename, partinfo);

	filedir = g_path_get_dirname(filename);
	if (filedir && strcmp(filedir, ".")) {
		g_free(prefs_common.attach_save_dir);
		prefs_common.attach_save_dir = g_filename_to_utf8(filedir, -1, NULL, NULL, NULL);
	}

	g_free(filedir);
	g_free(filepath);
}

static void mimeview_display_as_text(MimeView *mimeview)
{
	MimeInfo *partinfo;

	if (!mimeview->opened) return;

	partinfo = mimeview_get_part_to_use(mimeview);
	mimeview_select_mimepart_icon(mimeview, partinfo);
	g_return_if_fail(partinfo != NULL);
	mimeview_show_message_part(mimeview, partinfo);
}

static void mimeview_launch(MimeView *mimeview, MimeInfo *partinfo)
{
	gchar *filename;
	gint err;

	if (!mimeview->opened) return;
	if (!mimeview->file) return;

	if (!partinfo)
		partinfo = mimeview_get_part_to_use(mimeview);

	g_return_if_fail(partinfo != NULL);

	filename = procmime_get_tmp_file_name(partinfo);

	if ((err = procmime_get_part(filename, partinfo)) < 0)
		alertpanel_error
			(_("Couldn't save the part of multipart message: %s"), 
				strerror(-err));
	else
		mimeview_view_file(filename, partinfo, NULL, mimeview);

	g_free(filename);
}

static void mimeview_open_with(MimeView *mimeview)
{
	MimeInfo *partinfo;

	if (!mimeview) return;
	if (!mimeview->opened) return;
	if (!mimeview->file) return;

	partinfo = mimeview_get_part_to_use(mimeview);

	mimeview_open_part_with(mimeview, partinfo, FALSE);
}

#ifdef MAEMO
osso_context_t *get_osso_context(void);
#endif
static void mimeview_open_part_with(MimeView *mimeview, MimeInfo *partinfo, gboolean automatic)
{
	gchar *filename;
	gchar *cmd;
	gchar *mime_command = NULL;
	gchar *content_type = NULL;
	gint err;
#ifdef MAEMO
	DBusConnection *dbusconn;
	gchar *uri;
	int r = 0;
#endif
	g_return_if_fail(partinfo != NULL);

	filename = procmime_get_tmp_file_name(partinfo);

	if ((err = procmime_get_part(filename, partinfo)) < 0) {
		alertpanel_error
			(_("Couldn't save the part of multipart message: %s"), 
				strerror(-err));
		g_free(filename);
		return;
	}
	
	if (!prefs_common.mime_open_cmd_history)
		prefs_common.mime_open_cmd_history =
			add_history(NULL, prefs_common.mime_open_cmd);

	if ((partinfo->type == MIMETYPE_APPLICATION) &&
            (!g_ascii_strcasecmp(partinfo->subtype, "octet-stream"))) {
	    	/* guess content-type from filename */
	    	content_type = procmime_get_mime_type(filename);
	} 
	if (content_type == NULL) {
		content_type = procmime_get_content_type_str(partinfo->type,
			partinfo->subtype);
	}
	
#ifdef MAEMO
	if (content_type != NULL) {
		uri = g_filename_to_uri(filename, NULL, NULL);
		dbusconn = osso_get_dbus_connection (get_osso_context());
#ifdef CHINOOK
		r = hildon_mime_open_file_with_mime_type (dbusconn, uri, content_type);
#else
		r = osso_mime_open_file_with_mime_type (dbusconn, uri, content_type);
#endif
		g_free(uri);
	}
	if (r != 1) {
		alertpanel_error(_("No registered viewer for this file type."));
	}
	goto out;
#endif
	
	if ((partinfo->type == MIMETYPE_TEXT && !strcmp(partinfo->subtype, "html"))
	&& prefs_common_get_uri_cmd() && prefs_common.uri_cmd[0]) {
		mime_command = g_strdup(prefs_common_get_uri_cmd());
		g_free(content_type);
		content_type = NULL;
	} else if (partinfo->type != MIMETYPE_TEXT || !prefs_common_get_ext_editor_cmd()
	||  !prefs_common_get_ext_editor_cmd()[0]) {
		mime_command = mailcap_get_command_for_type(content_type, filename);
	} else {
		mime_command = g_strdup(prefs_common_get_ext_editor_cmd());
		g_free(content_type);
		content_type = NULL;
	}
	if (mime_command == NULL) {
		/* try with extension this time */
		g_free(content_type);
		content_type = procmime_get_mime_type(filename);
		mime_command = mailcap_get_command_for_type(content_type, filename);
	}

	if (mime_command == NULL)
		automatic = FALSE;
	
	if (!automatic) {
		gboolean remember = FALSE;
		if (content_type != NULL)
			cmd = input_dialog_combo_remember
				(_("Open with"),
				 _("Enter the command line to open file:\n"
				   "('%s' will be replaced with file name)"),
				 mime_command ? mime_command : prefs_common.mime_open_cmd,
				 prefs_common.mime_open_cmd_history, &remember);
		else
			cmd = input_dialog_combo
				(_("Open with"),
				 _("Enter the command line to open file:\n"
				   "('%s' will be replaced with file name)"),
				 mime_command ? mime_command : prefs_common.mime_open_cmd,
				 prefs_common.mime_open_cmd_history);
		if (cmd && remember) {
			mailcap_update_default(content_type, cmd);
		}
		g_free(mime_command);
	} else {
		cmd = mime_command;
	}
	if (cmd) {
		mimeview_view_file(filename, partinfo, cmd, mimeview);
		g_free(prefs_common.mime_open_cmd);
		prefs_common.mime_open_cmd = cmd;
		prefs_common.mime_open_cmd_history =
			add_history(prefs_common.mime_open_cmd_history, cmd);
	}
#ifdef MAEMO
out:
#endif
	g_free(content_type);
	g_free(filename);
}

static void mimeview_view_file(const gchar *filename, MimeInfo *partinfo,
			       const gchar *cmd, MimeView *mimeview)
{
	gchar *p;
	gchar buf[BUFFSIZE];
	if (cmd == NULL)
		mimeview_open_part_with(mimeview, partinfo, TRUE);
	else {
		if ((p = strchr(cmd, '%')) && *(p + 1) == 's' &&
		    !strchr(p + 2, '%')) {
			g_snprintf(buf, sizeof(buf), cmd, filename);
			if (!prefs_common.save_parts_readwrite)
				g_chmod(filename, S_IRUSR);
			else
				g_chmod(filename, S_IRUSR|S_IWUSR);
 		} else {
			g_warning("MIME viewer command line is invalid: '%s'", cmd);
			mimeview_open_part_with(mimeview, partinfo, FALSE);
 		}
		if (execute_command_line(buf, TRUE) != 0) {
			if (!prefs_common.save_parts_readwrite)
				g_chmod(filename, S_IRUSR|S_IWUSR);
			mimeview_open_part_with(mimeview, partinfo, FALSE);
		}
	}
}

void mimeview_register_viewer_factory(MimeViewerFactory *factory)
{
	mimeviewer_factories = g_slist_append(mimeviewer_factories, factory);
}

static gint cmp_viewer_by_factroy(gconstpointer a, gconstpointer b)
{
	return ((MimeViewer *) a)->factory == (MimeViewerFactory *) b ? 0 : -1;
}

void mimeview_unregister_viewer_factory(MimeViewerFactory *factory)
{
	GSList *mimeview_list, *viewer_list;

	for (mimeview_list = mimeviews; mimeview_list != NULL; mimeview_list = g_slist_next(mimeview_list)) {
		MimeView *mimeview = (MimeView *) mimeview_list->data;
		
		if (mimeview->mimeviewer && mimeview->mimeviewer->factory == factory) {
			mimeview_change_view_type(mimeview, MIMEVIEW_TEXT);
			mimeview->mimeviewer = NULL;
		}

		while ((viewer_list = g_slist_find_custom(mimeview->viewers, factory, cmp_viewer_by_factroy)) != NULL) {
			MimeViewer *mimeviewer = (MimeViewer *) viewer_list->data;

			mimeviewer->destroy_viewer(mimeviewer);
			mimeview->viewers = g_slist_remove(mimeview->viewers, mimeviewer);
		}
	}

	mimeviewer_factories = g_slist_remove(mimeviewer_factories, factory);
}

static gboolean icon_clicked_cb (GtkWidget *button, GdkEventButton *event, MimeView *mimeview)
{
	gint      num;
	MimeInfo *partinfo;

	num      = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "icon_number"));
	partinfo = g_object_get_data(G_OBJECT(button), "partinfo");

	if (event->button == 1) {
		icon_selected(mimeview, num, partinfo);
		gtk_widget_grab_focus(button);
	}
	part_button_pressed(mimeview, event, partinfo);
	icon_list_toggle_by_mime_info(mimeview, partinfo);

	return FALSE;
}

static void icon_selected (MimeView *mimeview, gint num, MimeInfo *partinfo)
{
	GtkCTreeNode *node;
	node = gtk_ctree_find_by_row_data(GTK_CTREE(mimeview->ctree), NULL, partinfo);

	if (node)
		gtk_ctree_select(GTK_CTREE(mimeview->ctree), node);
}		

void mimeview_select_mimepart_icon(MimeView *mimeview, MimeInfo *partinfo)
{
	icon_list_toggle_by_mime_info(mimeview, partinfo);
	icon_selected(mimeview, -1, partinfo);
}

static gint icon_key_pressed(GtkWidget *button, GdkEventKey *event,
			     MimeView *mimeview)
{
	gint          num;
	MimeInfo     *partinfo;
	SummaryView  *summaryview;
	TextView     *textview;

	num      = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "icon_number"));
	partinfo = g_object_get_data(G_OBJECT(button), "partinfo");
	
	if (!event) return FALSE;

	textview = mimeview->textview;

	switch (event->keyval) {
	case GDK_space:
		if (mimeview_scroll_page(mimeview, FALSE))
			return TRUE;

		mimeview_select_next_part(mimeview);
		return TRUE;

		break;
	case GDK_BackSpace:
		mimeview_scroll_page(mimeview, TRUE);
		return TRUE;
	case GDK_Return:
	case GDK_KP_Enter:
		mimeview_scroll_one_line(mimeview,
					 (event->state & GDK_MOD1_MASK) != 0);
		return TRUE;

	case GDK_n:
	case GDK_N:
		BREAK_ON_MODIFIER_KEY();
		mimeview_select_next_part(mimeview);
		return TRUE;
		
	case GDK_p:
	case GDK_P:
		BREAK_ON_MODIFIER_KEY();
		mimeview_select_prev_part(mimeview);
		break;

	case GDK_y:
		BREAK_ON_MODIFIER_KEY();
		mimeview_save_as(mimeview);
		return TRUE;
	case GDK_t:
		BREAK_ON_MODIFIER_KEY();
		mimeview_display_as_text(mimeview);
		return TRUE;	
	case GDK_l:
		BREAK_ON_MODIFIER_KEY();
		mimeview_launch(mimeview, NULL);
		return TRUE;
	case GDK_o:
		BREAK_ON_MODIFIER_KEY();
		mimeview_open_with(mimeview);
		return TRUE;
	case GDK_c:
		BREAK_ON_MODIFIER_KEY();
		mimeview_check_signature(mimeview);
		return TRUE;
	case GDK_a:
		BREAK_ON_MODIFIER_KEY();
		mimeview_select_next_part(mimeview);
		return TRUE;
	default:
		break;
	}

	if (!mimeview->messageview->mainwin) return FALSE;
	summaryview = mimeview->messageview->mainwin->summaryview;
	return summary_pass_key_press_event(summaryview, event);
}

static gboolean icon_popup_menu(GtkWidget *widget, gpointer data)
{
	MimeView *mimeview = (MimeView *)data;
	MimeInfo *partinfo = g_object_get_data(G_OBJECT(widget), "partinfo");

	g_object_set_data(G_OBJECT(mimeview->popupmenu),
			  "pop_partinfo", partinfo);
	gtk_menu_popup(GTK_MENU(mimeview->popupmenu),
		       NULL, NULL, NULL, NULL,
		       0, gtk_get_current_event_time());
	return TRUE;
}

static void icon_list_append_icon (MimeView *mimeview, MimeInfo *mimeinfo) 
{
	GtkWidget *pixmap = NULL;
	GtkWidget *vbox;
	GtkWidget *button;
	gchar *tip;
	gchar *tiptmp;
	const gchar *desc = NULL; 
	gchar *sigshort = NULL;
	gchar *content_type;
	StockPixmap stockp;
	MimeInfo *partinfo;
	MimeInfo *siginfo = NULL;
	MimeInfo *encrypted = NULL;
	
	vbox = mimeview->icon_vbox;
	mimeview->icon_count++;
	button = gtk_event_box_new();

	g_signal_connect(G_OBJECT(button), "visibility-notify-event",
			 G_CALLBACK(mimeview_visi_notify), mimeview);
	g_signal_connect(G_OBJECT(button), "motion-notify-event",
			 G_CALLBACK(mimeview_visi_notify), mimeview);
	g_signal_connect(G_OBJECT(button), "leave-notify-event",
			 G_CALLBACK(mimeview_leave_notify), mimeview);
	g_signal_connect(G_OBJECT(button), "enter-notify-event",
			 G_CALLBACK(mimeview_enter_notify), mimeview);

	gtk_container_set_border_width(GTK_CONTAINER(button), 2);
	g_object_set_data(G_OBJECT(button), "icon_number", 
			  GINT_TO_POINTER(mimeview->icon_count));
	g_object_set_data(G_OBJECT(button), "partinfo", 
		          mimeinfo);

	switch (mimeinfo->type) {
		
	case MIMETYPE_TEXT:
		if (mimeinfo->subtype && !g_ascii_strcasecmp(mimeinfo->subtype, "html"))
			stockp = STOCK_PIXMAP_MIME_TEXT_HTML;
		else if  (mimeinfo->subtype && !g_ascii_strcasecmp(mimeinfo->subtype, "enriched"))
			stockp = STOCK_PIXMAP_MIME_TEXT_ENRICHED;
		else if  (mimeinfo->subtype && !g_ascii_strcasecmp(mimeinfo->subtype, "calendar"))
			stockp = STOCK_PIXMAP_MIME_TEXT_CALENDAR;
		else if  (mimeinfo->subtype && (!g_ascii_strcasecmp(mimeinfo->subtype, "x-patch")
				|| !g_ascii_strcasecmp(mimeinfo->subtype, "x-diff")))
			stockp = STOCK_PIXMAP_MIME_TEXT_PATCH;
		else
			stockp = STOCK_PIXMAP_MIME_TEXT_PLAIN;
		break;
	case MIMETYPE_MESSAGE:
		stockp = STOCK_PIXMAP_MIME_MESSAGE;
		break;
	case MIMETYPE_APPLICATION:
		if (mimeinfo->subtype && !g_ascii_strcasecmp(mimeinfo->subtype, "pgp-signature"))
			stockp = STOCK_PIXMAP_MIME_PGP_SIG;
		else if (mimeinfo->subtype && !g_ascii_strcasecmp(mimeinfo->subtype, "pdf"))
			stockp = STOCK_PIXMAP_MIME_PDF;
		else if  (mimeinfo->subtype && !g_ascii_strcasecmp(mimeinfo->subtype, "postscript"))
			stockp = STOCK_PIXMAP_MIME_PS;
		else
			stockp = STOCK_PIXMAP_MIME_APPLICATION;
		break;
	case MIMETYPE_IMAGE:
		stockp = STOCK_PIXMAP_MIME_IMAGE;
		break;
	case MIMETYPE_AUDIO:
		stockp = STOCK_PIXMAP_MIME_AUDIO;
		break;
	default:
		stockp = STOCK_PIXMAP_MIME_UNKNOWN;
		break;
	}
	
	partinfo = mimeinfo;
	while (partinfo != NULL) {
		if (privacy_mimeinfo_is_signed(partinfo)) {
			siginfo = partinfo;
			break;
		}
		if (privacy_mimeinfo_is_encrypted(partinfo)) {
			encrypted = partinfo;
			break;
		}
		partinfo = procmime_mimeinfo_parent(partinfo);
	}	

	if (siginfo != NULL) {
		switch (privacy_mimeinfo_get_sig_status(siginfo)) {
		case SIGNATURE_UNCHECKED:
		case SIGNATURE_CHECK_FAILED:
		case SIGNATURE_CHECK_TIMEOUT:
			pixmap = stock_pixmap_widget_with_overlay(mimeview->mainwin->window, stockp,
			    STOCK_PIXMAP_PRIVACY_EMBLEM_SIGNED, OVERLAY_BOTTOM_RIGHT, 6, 3);
			break;
		case SIGNATURE_OK:
			pixmap = stock_pixmap_widget_with_overlay(mimeview->mainwin->window, stockp,
			    STOCK_PIXMAP_PRIVACY_EMBLEM_PASSED, OVERLAY_BOTTOM_RIGHT, 6, 3);
			break;
		case SIGNATURE_WARN:
			pixmap = stock_pixmap_widget_with_overlay(mimeview->mainwin->window, stockp,
			    STOCK_PIXMAP_PRIVACY_EMBLEM_WARN, OVERLAY_BOTTOM_RIGHT, 6, 3);
			break;
		case SIGNATURE_INVALID:
			pixmap = stock_pixmap_widget_with_overlay(mimeview->mainwin->window, stockp,
			    STOCK_PIXMAP_PRIVACY_EMBLEM_FAILED, OVERLAY_BOTTOM_RIGHT, 6, 3);
			break;
		}
		sigshort = privacy_mimeinfo_sig_info_short(siginfo);
	} else if (encrypted != NULL) {
			pixmap = stock_pixmap_widget_with_overlay(mimeview->mainwin->window, stockp,
			    STOCK_PIXMAP_PRIVACY_EMBLEM_ENCRYPTED, OVERLAY_BOTTOM_RIGHT, 6, 3);		
	} else {
		pixmap = stock_pixmap_widget_with_overlay(mimeview->mainwin->window, stockp, 0,
							  OVERLAY_NONE, 6, 3);
	}
	gtk_container_add(GTK_CONTAINER(button), pixmap);
	if (!desc) {
		if (prefs_common.attach_desc)
			desc = get_part_description(mimeinfo);
		else
			desc = get_part_name(mimeinfo);
	}

	content_type = procmime_get_content_type_str(mimeinfo->type,
						     mimeinfo->subtype);

	tip = g_strjoin("\n", content_type,
			to_human_readable(mimeinfo->length), NULL);
	g_free(content_type);
	if (desc && *desc) {
		gchar *tmp = NULL;
		if (!g_utf8_validate(desc, -1, NULL)) {
			tmp = conv_filename_to_utf8(desc);
		} else {
			tmp = g_strdup(desc);
		}
		tiptmp = g_strjoin("\n", tmp, tip, NULL);
		g_free(tip);
		tip = tiptmp;
		g_free(tmp);
	}
	if (sigshort && *sigshort) {
		tiptmp = g_strjoin("\n", tip, sigshort, NULL);
		g_free(tip);
		tip = tiptmp;
	}
	g_free(sigshort);

	gtk_tooltips_set_tip(mimeview->tooltips, button, tip, NULL);
	g_free(tip);
	gtk_widget_show_all(button);
	gtk_drag_source_set(button, GDK_BUTTON1_MASK|GDK_BUTTON3_MASK, 
			    mimeview_mime_types, 1, GDK_ACTION_COPY);
#ifndef MAEMO
	g_signal_connect(G_OBJECT(button), "popup-menu",
			 G_CALLBACK(icon_popup_menu), mimeview);
#else
	gtk_widget_tap_and_hold_setup(GTK_WIDGET(button), NULL, NULL,
			GTK_TAP_AND_HOLD_NONE | GTK_TAP_AND_HOLD_NO_INTERNALS);
	g_signal_connect(G_OBJECT(button), "tap-and-hold",
			 G_CALLBACK(icon_popup_menu), mimeview);
#endif
	g_signal_connect(G_OBJECT(button), "button_release_event", 
			 G_CALLBACK(icon_clicked_cb), mimeview);
	g_signal_connect(G_OBJECT(button), "key_press_event", 
			 G_CALLBACK(icon_key_pressed), mimeview);
	g_signal_connect(G_OBJECT(button), "drag_data_get",
			 G_CALLBACK(mimeview_drag_data_get), mimeview);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
#ifdef GENERIC_UMPC
	gtk_widget_set_size_request(vbox, -1, pixmap->requisition.height + 8);
	gtk_widget_set_size_request(button, -1, pixmap->requisition.height + 4);
#endif

}

static void icon_list_clear (MimeView *mimeview)
{
	GList     *child;
	GtkAdjustment *adj;
		
	child = gtk_container_children(GTK_CONTAINER(mimeview->icon_vbox));
	for (; child != NULL; child = g_list_next(child)) {
		gtkut_container_remove(GTK_CONTAINER(mimeview->icon_vbox), 
				       GTK_WIDGET(child->data));
		gtk_widget_destroy(GTK_WIDGET(child->data));
	}
	g_list_free(child);
	mimeview->icon_count = 0;
	adj  = gtk_layout_get_vadjustment(GTK_LAYOUT(mimeview->icon_scroll));
	gtk_adjustment_set_value(adj, adj->lower);
	gtk_adjustment_changed(adj);
}

/*!
 *\brief        Used to 'click' the next or previous icon.
 *
 *\return       true if the icon 'number' exists and was selected.
 */
static void icon_scroll_size_allocate_cb(GtkWidget *widget, 
					 GtkAllocation *size, MimeView *mimeview)
{
	GtkAllocation *mainbox_size;
	GtkAllocation *vbox_size;
	GtkAllocation *layout_size;
	GtkAdjustment *adj;
	
	adj = gtk_layout_get_vadjustment(GTK_LAYOUT(mimeview->icon_scroll));

	mainbox_size = &mimeview->icon_mainbox->allocation;
	vbox_size = &mimeview->icon_vbox->allocation;
	layout_size = &mimeview->icon_scroll->allocation;
		
	gtk_layout_set_size(GTK_LAYOUT(mimeview->icon_scroll), 
			    GTK_LAYOUT(mimeview->icon_scroll)->width, 
			    MAX(vbox_size->height, layout_size->height));
	adj->step_increment = 10;
}

static void icon_list_create(MimeView *mimeview, MimeInfo *mimeinfo)
{
	GtkRequisition size;

	g_return_if_fail(mimeinfo != NULL);

	while (mimeinfo != NULL) {
		if (mimeinfo->type != MIMETYPE_MULTIPART)
			icon_list_append_icon(mimeview, mimeinfo);
		if (mimeinfo->node->children != NULL)
			icon_list_create(mimeview, 
				(MimeInfo *) mimeinfo->node->children->data);
		mimeinfo = mimeinfo->node->next != NULL 
			 ? (MimeInfo *) mimeinfo->node->next->data 
			 : NULL;
	}
	gtk_widget_size_request(mimeview->icon_vbox, &size);
	if (size.width > mimeview->icon_mainbox->requisition.width) {
		gtk_widget_set_size_request(mimeview->icon_mainbox, 
					    size.width, -1);
	}
}

static void icon_list_toggle_by_mime_info (MimeView	*mimeview,
					   MimeInfo	*mimeinfo)
{
	GList *child;
	
	child = gtk_container_children(GTK_CONTAINER(mimeview->icon_vbox));
	for (; child != NULL; child = g_list_next(child)) {
		if (!GTK_IS_EVENT_BOX(child->data))
			continue;
		if(g_object_get_data(G_OBJECT(child->data),
			 	      "partinfo") == (gpointer)mimeinfo) {
			gboolean *highlight = NULL;
			GtkWidget *icon = gtk_bin_get_child(GTK_BIN(child->data));
			highlight = g_object_get_data(G_OBJECT(icon), "highlight");
			*highlight = TRUE;
			gtk_widget_queue_draw(icon);
		} else {
			gint *highlight = NULL;
			GtkWidget *icon = gtk_bin_get_child(GTK_BIN(child->data));
			highlight = g_object_get_data(G_OBJECT(icon), "highlight");
			*highlight = FALSE;
			gtk_widget_queue_draw(icon);
		}			 
	}
}

static gint mime_toggle_button_cb(GtkWidget *button, GdkEventButton *event,
				    MimeView *mimeview)
{
	gtk_widget_ref(button); 

	mimeview_leave_notify(button, NULL, NULL);

	mimeview->ctree_mode = !mimeview->ctree_mode;
	if (mimeview->ctree_mode) {
		gtk_arrow_set(GTK_ARROW(GTK_BIN(button)->child), GTK_ARROW_RIGHT, 
					GTK_SHADOW_NONE);
		gtk_widget_hide(mimeview->icon_mainbox);
		gtk_widget_show(mimeview->ctree_mainbox);
		gtk_paned_set_position(GTK_PANED(mimeview->paned), mimeview->oldsize);

		gtkut_container_remove(GTK_CONTAINER(mimeview->icon_mainbox), 
					button);
		gtk_box_pack_end(GTK_BOX(mimeview->ctree_mainbox), 
				   button, FALSE, FALSE, 0);
		gtk_paned_set_gutter_size(GTK_PANED(mimeview->paned), 6);
	} else {
		gtk_arrow_set(GTK_ARROW(GTK_BIN(button)->child), GTK_ARROW_LEFT, 
			      GTK_SHADOW_NONE);
		mimeview->oldsize = mimeview->ctree_mainbox->allocation.height;
		gtk_widget_hide(mimeview->ctree_mainbox);
		gtk_widget_show(mimeview->icon_mainbox);
		gtk_paned_set_position(GTK_PANED(mimeview->paned), 0);

		gtkut_container_remove(GTK_CONTAINER(mimeview->ctree_mainbox), 
					button);
		gtk_box_pack_start(GTK_BOX(mimeview->icon_mainbox), 
				   button, FALSE, FALSE, 0);
		gtk_box_reorder_child(GTK_BOX(button->parent), button, 0);
		if (mimeview->opened)
			icon_list_toggle_by_mime_info
				(mimeview, gtk_ctree_node_get_row_data(GTK_CTREE(mimeview->ctree), 
								       mimeview->opened));

		gtk_paned_set_gutter_size(GTK_PANED(mimeview->paned), 0);
	}
	gtk_widget_unref(button);
	return TRUE;
}

void mimeview_update (MimeView *mimeview) {
	if (mimeview && mimeview->mimeinfo) {
		icon_list_clear(mimeview);
		icon_list_create(mimeview, mimeview->mimeinfo);
	}
}

void mimeview_handle_cmd(MimeView *mimeview, const gchar *cmd, GdkEventButton *event, gpointer data)
{
	MessageView *msgview = NULL;
	MainWindow *mainwin = NULL;
	
	if (!cmd)
		return;
	
	msgview = mimeview->messageview;
	if (!msgview)
		return;
		
	mainwin = msgview->mainwin;
	if (!mainwin)
		return;
		
	g_object_set_data(G_OBJECT(mimeview->popupmenu),
			  "pop_partinfo", NULL);

	if (!strcmp(cmd, "sc://view_log"))
		log_window_show(mainwin->logwin);
	else if (!strcmp(cmd, "sc://save_as"))
		mimeview_save_as(mimeview);
	else if (!strcmp(cmd, "sc://display_as_text"))
		mimeview_display_as_text(mimeview);
	else if (!strcmp(cmd, "sc://open_with"))
		mimeview_open_with(mimeview);
	else if (!strcmp(cmd, "sc://open"))
		mimeview_launch(mimeview, NULL);
	else if (!strcmp(cmd, "sc://select_attachment") && data != NULL) {
		icon_list_toggle_by_mime_info(mimeview, (MimeInfo *)data);
		icon_selected(mimeview, -1, (MimeInfo *)data);
	} else if (!strcmp(cmd, "sc://open_attachment") && data != NULL) {
		mimeview_launch(mimeview, (MimeInfo *)data);
	} else if (!strcmp(cmd, "sc://menu_attachment") && data != NULL) {
		mimeview->spec_part = (MimeInfo *)data;
		part_button_pressed(mimeview, event, (MimeInfo *)data);
	} else if (!strncmp(cmd, "sc://search_tags:", strlen("sc://search_tags:"))) {
		const gchar *tagname = cmd + strlen("sc://search_tags:");
		gchar *buf = g_strdup_printf("tag matchcase \"%s\"", tagname);
		gtk_toggle_button_set_active(
				GTK_TOGGLE_BUTTON(mimeview->messageview->mainwin->summaryview->toggle_search), 
				TRUE);
		quicksearch_set(mimeview->messageview->mainwin->summaryview->quicksearch, 
				QUICK_SEARCH_EXTENDED, buf);
		g_free(buf);
	}
}

gboolean mimeview_scroll_page(MimeView *mimeview, gboolean up)
{
	if (mimeview->type == MIMEVIEW_TEXT)
		return textview_scroll_page(mimeview->textview, up);
	else if (mimeview->mimeviewer) {
		MimeViewer *mimeviewer = mimeview->mimeviewer;
		if (mimeviewer->scroll_page)
			return mimeviewer->scroll_page(mimeviewer, up);
	}
	return TRUE;
}

void mimeview_scroll_one_line(MimeView *mimeview, gboolean up)
{
	if (mimeview->type == MIMEVIEW_TEXT)
		textview_scroll_one_line(mimeview->textview, up);
	else if (mimeview->mimeviewer) {
		MimeViewer *mimeviewer = mimeview->mimeviewer;
		if (mimeviewer->scroll_one_line)
			mimeviewer->scroll_one_line(mimeviewer, up);
	}
}
