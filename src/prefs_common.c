/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2005 Hiroyuki Yamamoto
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "defs.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "main.h"
#include "prefs_gtk.h"
#include "prefs_common.h"
#include "prefs_display_header.h"
#include "prefs_summary_column.h"
#include "prefs_folder_column.h"
#include "mainwindow.h"
#include "summaryview.h"
#include "folderview.h"
#include "messageview.h"
#include "manage_window.h"
#include "inc.h"
#include "menu.h"
#include "codeconv.h"
#include "utils.h"
#include "gtkutils.h"
#include "alertpanel.h"
#include "folder.h"
#include "socket.h"
#include "filesel.h"
#include "folderview.h"
#include "stock_pixmap.h"
#include "prefswindow.h"

enum {
	DATEFMT_FMT,
	DATEFMT_TXT,
	N_DATEFMT_COLUMNS
};

PrefsCommon prefs_common;

GtkWidget *notebook;

/*
   parameter name, default value, pointer to the prefs variable, data type,
   pointer to the widget pointer,
   pointer to the function for data setting,
   pointer to the function for widget setting
 */

static PrefParam param[] = {
	/* Receive */
	{"use_ext_inc", "FALSE", &prefs_common.use_extinc, P_BOOL,
	 NULL, NULL, NULL},
	{"ext_inc_path", DEFAULT_INC_PATH, &prefs_common.extinc_cmd, P_STRING,
	 NULL, NULL, NULL},

	{"autochk_newmail", "FALSE", &prefs_common.autochk_newmail, P_BOOL,
	 NULL, NULL, NULL},
	{"autochk_interval", "10", &prefs_common.autochk_itv, P_INT,
	 NULL, NULL, NULL},
	{"check_on_startup", "FALSE", &prefs_common.chk_on_startup, P_BOOL,
	 NULL, NULL, NULL},
	{"scan_all_after_inc", "FALSE", &prefs_common.scan_all_after_inc,
	 P_BOOL, NULL, NULL, NULL},
	{"newmail_notify_manu", "FALSE", &prefs_common.newmail_notify_manu,
	 P_BOOL, NULL, NULL, NULL},
 	{"newmail_notify_auto", "FALSE", &prefs_common.newmail_notify_auto,
	P_BOOL, NULL, NULL, NULL},
 	{"newmail_notify_cmd", "", &prefs_common.newmail_notify_cmd, P_STRING,
 	 NULL, NULL, NULL},
	{"receive_dialog_mode", "1", &prefs_common.recv_dialog_mode, P_ENUM,
	 NULL, NULL, NULL},
	{"no_receive_error_panel", "FALSE", &prefs_common.no_recv_err_panel,
	 P_BOOL, NULL, NULL, NULL},
	{"close_receive_dialog", "TRUE", &prefs_common.close_recv_dialog,
	 P_BOOL, NULL, NULL, NULL},
 
	/* Send */
	{"save_message", "TRUE", &prefs_common.savemsg, P_BOOL,
	 NULL, NULL, NULL},
	{"confirm_send_queued_messages", "FALSE", &prefs_common.confirm_send_queued_messages,
	 P_BOOL, NULL, NULL, NULL},
	{"send_dialog_mode", "0", &prefs_common.send_dialog_mode, P_ENUM,
	 NULL, NULL, NULL},

	{"outgoing_charset", CS_AUTO, &prefs_common.outgoing_charset, P_STRING,
	 NULL, NULL, NULL},
	{"encoding_method", "0", &prefs_common.encoding_method, P_ENUM,
	 NULL, NULL, NULL},

	{"allow_jisx0201_kana", "FALSE", &prefs_common.allow_jisx0201_kana,
	 P_BOOL, NULL, NULL, NULL},

	/* Compose */
	{"auto_ext_editor", "FALSE", &prefs_common.auto_exteditor, P_BOOL,
	 NULL, NULL, NULL},
	{"forward_as_attachment", "FALSE", &prefs_common.forward_as_attachment,
	 P_BOOL, NULL, NULL, NULL},
	{"redirect_keep_from", "FALSE",
	 &prefs_common.redirect_keep_from, P_BOOL,
	 NULL, NULL, NULL},
	{"undo_level", "50", &prefs_common.undolevels, P_INT,
	 NULL, NULL, NULL},

	{"linewrap_length", "72", &prefs_common.linewrap_len, P_INT,
	 NULL, NULL, NULL},
	{"linewrap_quotation", "TRUE", &prefs_common.linewrap_quote, P_BOOL,
	 NULL, NULL, NULL},
	{"linewrap_auto", "TRUE", &prefs_common.autowrap, P_BOOL,
	 NULL, NULL, NULL},
	{"linewrap_before_sending", "FALSE", &prefs_common.linewrap_at_send, P_BOOL, 
	 NULL, NULL, NULL},
        {"autosave", "FALSE", &prefs_common.autosave,
	 P_BOOL, NULL, NULL, NULL},
        {"autosave_length", "50", &prefs_common.autosave_length, P_INT,
	 NULL, NULL, NULL},
#if USE_ASPELL
	{"enable_aspell", "TRUE", &prefs_common.enable_aspell, P_BOOL,
	 NULL, NULL, NULL},
	{"aspell_path", ASPELL_PATH, &prefs_common.aspell_path, P_STRING,
	 NULL, NULL, NULL},
	{"dictionary",  "", &prefs_common.dictionary, P_STRING,
	 NULL, NULL, NULL},
	{"aspell_sugmode", "1", &prefs_common.aspell_sugmode, P_INT,
	 NULL, NULL, NULL},
	{"use_alternate_dict", "FALSE", &prefs_common.use_alternate, P_BOOL,
	 NULL, NULL, NULL},
	{"check_while_typing", "TRUE", &prefs_common.check_while_typing, P_BOOL,
	 NULL, NULL, NULL},
	{"misspelled_color", "16711680", &prefs_common.misspelled_col, P_COLOR,
	 NULL, NULL, NULL},
#endif
	{"reply_with_quote", "TRUE", &prefs_common.reply_with_quote, P_BOOL,
	 NULL, NULL, NULL},

	/* Account autoselection */
	{"reply_account_autoselect", "TRUE",
	 &prefs_common.reply_account_autosel, P_BOOL,
	 NULL, NULL, NULL},
	{"forward_account_autoselect", "TRUE",
	 &prefs_common.forward_account_autosel, P_BOOL,
	 NULL, NULL, NULL},
	{"reedit_account_autoselect", "TRUE",
	 &prefs_common.reedit_account_autosel, P_BOOL,
	 NULL, NULL, NULL},

	{"default_reply_list", "TRUE", &prefs_common.default_reply_list, P_BOOL,
	 NULL, NULL, NULL},

	{"show_ruler", "TRUE", &prefs_common.show_ruler, P_BOOL,
	 NULL, NULL, NULL},

	/* Quote */
	{"reply_quote_mark", "> ", &prefs_common.quotemark, P_STRING,
	 NULL, NULL, NULL},
	{"reply_quote_format", "On %d\\n%f wrote:\\n\\n%q",
	 &prefs_common.quotefmt, P_STRING, NULL, NULL, NULL},

	{"forward_quote_mark", "> ", &prefs_common.fw_quotemark, P_STRING,
	 NULL, NULL, NULL},
	{"forward_quote_format",
	 "\\n\\nBegin forwarded message:\\n\\n"
	 "?d{Date: %d\\n}?f{From: %f\\n}?t{To: %t\\n}?c{Cc: %c\\n}"
	 "?n{Newsgroups: %n\\n}?s{Subject: %s\\n}\\n\\n%M",
	 &prefs_common.fw_quotefmt, P_STRING, NULL, NULL, NULL},
	{"quote_chars", ">", &prefs_common.quote_chars, P_STRING,
	 NULL, NULL, NULL},

	/* Display */
	/* Obsolete fonts. For coexisting with Gtk+-1.2 version */
	{"widget_font",		NULL,
	  &prefs_common.widgetfont_gtk1,	P_STRING, NULL, NULL, NULL},
	{"message_font",	"-misc-fixed-medium-r-normal--14-*-*-*-*-*-*-*",
	 &prefs_common.textfont_gtk1,		P_STRING, NULL, NULL, NULL},
	{"small_font",		"-*-helvetica-medium-r-normal--10-*-*-*-*-*-*-*",
	  &prefs_common.smallfont_gtk1,		P_STRING, NULL, NULL, NULL},
	{"bold_font",		"-*-helvetica-bold-r-normal--12-*-*-*-*-*-*-*",
	  &prefs_common.boldfont_gtk1,		P_STRING, NULL, NULL, NULL},
	{"normal_font",		"-*-helvetica-medium-r-normal--12-*-*-*-*-*-*-*",
	  &prefs_common.normalfont_gtk1,	P_STRING, NULL, NULL, NULL},

	/* new fonts */
	{"widget_font_gtk2",	NULL,
	  &prefs_common.widgetfont,		P_STRING, NULL, NULL, NULL},
	{"message_font_gtk2",	"Monospace 9",
	 &prefs_common.textfont,		P_STRING, NULL, NULL, NULL},
	{"small_font_gtk2",	"Sans 9",
	  &prefs_common.smallfont,		P_STRING, NULL, NULL, NULL},
	{"bold_font_gtk2",	"Sans Bold 9",
	  &prefs_common.boldfont,		P_STRING, NULL, NULL, NULL},
	{"normal_font_gtk2",	"Sans 9", 
	  &prefs_common.normalfont,		P_STRING, NULL, NULL, NULL},

	/* image viewer */
	{"display_image", "TRUE", &prefs_common.display_img, P_BOOL,
	 NULL, NULL, NULL},
	{"resize_image", "TRUE", &prefs_common.resize_img, P_BOOL,
	 NULL, NULL, NULL},
	{"inline_image", "TRUE", &prefs_common.inline_img, P_BOOL,
	 NULL, NULL, NULL},

	{"display_folder_unread_num", "TRUE",
	 &prefs_common.display_folder_unread, P_BOOL,
	 NULL, NULL, NULL},
	{"newsgroup_abbrev_len", "16",
	 &prefs_common.ng_abbrev_len, P_INT,
	 NULL, NULL, NULL},

	{"translate_header", "TRUE", &prefs_common.trans_hdr, P_BOOL,
	 NULL, NULL, NULL},

	/* Display: Summary View */
	{"use_address_book", "FALSE", &prefs_common.use_addr_book, P_BOOL,
	 NULL, NULL, NULL},
	{"thread_by_subject", "TRUE", &prefs_common.thread_by_subject, P_BOOL,
	 NULL, NULL, NULL},
	{"date_format", "%y/%m/%d(%a) %H:%M", &prefs_common.date_format,
	 P_STRING, NULL, NULL, NULL},

	{"enable_rules_hint", "TRUE", &prefs_common.enable_rules_hint, P_BOOL,
	 NULL, NULL, NULL},
	{"enable_hscrollbar", "TRUE", &prefs_common.enable_hscrollbar, P_BOOL,
	 NULL, NULL, NULL},
	{"bold_unread", "TRUE", &prefs_common.bold_unread, P_BOOL,
	 NULL, NULL, NULL},
	{"thread_by_subject_max_age", "10", &prefs_common.thread_by_subject_max_age,
	P_INT, NULL, NULL, NULL },

	{"enable_thread", "TRUE", &prefs_common.enable_thread, P_BOOL,
	 NULL, NULL, NULL},
	{"toolbar_style", "3", &prefs_common.toolbar_style, P_ENUM,
	 NULL, NULL, NULL},
	{"toolbar_detachable", "TRUE", &prefs_common.toolbar_detachable, P_BOOL,
	 NULL, NULL, NULL},
	{"show_statusbar", "TRUE", &prefs_common.show_statusbar, P_BOOL,
	 NULL, NULL, NULL},
	{"show_searchbar", "TRUE", &prefs_common.show_searchbar, P_BOOL,
	 NULL, NULL, NULL},

	{"folderview_vscrollbar_policy", "0",
	 &prefs_common.folderview_vscrollbar_policy, P_ENUM,
	 NULL, NULL, NULL},

	{"summary_col_show_mark", "TRUE",
	 &prefs_common.summary_col_visible[S_COL_MARK], P_BOOL, NULL, NULL, NULL},
	{"summary_col_show_unread", "TRUE",
	 &prefs_common.summary_col_visible[S_COL_STATUS], P_BOOL, NULL, NULL, NULL},
	{"summary_col_show_mime", "TRUE",
	 &prefs_common.summary_col_visible[S_COL_MIME], P_BOOL, NULL, NULL, NULL},
	{"summary_col_show_subject", "TRUE",
	 &prefs_common.summary_col_visible[S_COL_SUBJECT], P_BOOL, NULL, NULL, NULL},
	{"summary_col_show_from", "TRUE",
	 &prefs_common.summary_col_visible[S_COL_FROM], P_BOOL, NULL, NULL, NULL},
	{"summary_col_show_to", "FALSE",
	 &prefs_common.summary_col_visible[S_COL_TO], P_BOOL, NULL, NULL, NULL},
	{"summary_col_show_date", "TRUE",
	 &prefs_common.summary_col_visible[S_COL_DATE], P_BOOL, NULL, NULL, NULL},
	{"summary_col_show_size", "TRUE",
	 &prefs_common.summary_col_visible[S_COL_SIZE], P_BOOL, NULL, NULL, NULL},
	{"summary_col_show_number", "FALSE",
	 &prefs_common.summary_col_visible[S_COL_NUMBER], P_BOOL, NULL, NULL, NULL},
	{"summary_col_show_score", "FALSE",
	 &prefs_common.summary_col_visible[S_COL_SCORE], P_BOOL, NULL, NULL, NULL},
	{"summary_col_show_locked", "FALSE",
	 &prefs_common.summary_col_visible[S_COL_LOCKED], P_BOOL, NULL, NULL, NULL},

	{"summary_col_pos_mark", "0",
	  &prefs_common.summary_col_pos[S_COL_MARK], P_INT, NULL, NULL, NULL},
	{"summary_col_pos_unread", "1",
	  &prefs_common.summary_col_pos[S_COL_STATUS], P_INT, NULL, NULL, NULL},
	{"summary_col_pos_mime", "2",
	  &prefs_common.summary_col_pos[S_COL_MIME], P_INT, NULL, NULL, NULL},
	{"summary_col_pos_subject", "3",
	  &prefs_common.summary_col_pos[S_COL_SUBJECT], P_INT, NULL, NULL, NULL},
	{"summary_col_pos_from", "4",
	  &prefs_common.summary_col_pos[S_COL_FROM], P_INT, NULL, NULL, NULL},
	{"summary_col_pos_date", "5",
	  &prefs_common.summary_col_pos[S_COL_DATE], P_INT, NULL, NULL, NULL},
	{"summary_col_pos_size", "6",
	  &prefs_common.summary_col_pos[S_COL_SIZE], P_INT, NULL, NULL, NULL},
	{"summary_col_pos_number", "7",
	  &prefs_common.summary_col_pos[S_COL_NUMBER], P_INT, NULL, NULL, NULL},
	{"summary_col_pos_score", "8",
	 &prefs_common.summary_col_pos[S_COL_SCORE], P_INT, NULL, NULL, NULL},
	{"summary_col_pos_locked", "9",
	 &prefs_common.summary_col_pos[S_COL_LOCKED], P_INT, NULL, NULL, NULL},
	{"summary_col_pos_to", "10",
	  &prefs_common.summary_col_pos[S_COL_TO], P_INT, NULL, NULL, NULL},

	{"summary_col_size_mark", "10",
	 &prefs_common.summary_col_size[S_COL_MARK], P_INT, NULL, NULL, NULL},
	{"summary_col_size_unread", "13",
	 &prefs_common.summary_col_size[S_COL_STATUS], P_INT, NULL, NULL, NULL},
	{"summary_col_size_mime", "10",
	 &prefs_common.summary_col_size[S_COL_MIME], P_INT, NULL, NULL, NULL},
	{"summary_col_size_subject", "200",
	 &prefs_common.summary_col_size[S_COL_SUBJECT], P_INT, NULL, NULL, NULL},
	{"summary_col_size_from", "120",
	 &prefs_common.summary_col_size[S_COL_FROM], P_INT, NULL, NULL, NULL},
	{"summary_col_size_to", "120",
	 &prefs_common.summary_col_size[S_COL_TO], P_INT, NULL, NULL, NULL},
	{"summary_col_size_date", "118",
	 &prefs_common.summary_col_size[S_COL_DATE], P_INT, NULL, NULL, NULL},
	{"summary_col_size_size", "45",
	 &prefs_common.summary_col_size[S_COL_SIZE], P_INT, NULL, NULL, NULL},
	{"summary_col_size_number", "40",
	 &prefs_common.summary_col_size[S_COL_NUMBER], P_INT, NULL, NULL, NULL},
	{"summary_col_size_score", "40",
	 &prefs_common.summary_col_size[S_COL_SCORE], P_INT, NULL, NULL, NULL},
	{"summary_col_size_locked", "13",
	 &prefs_common.summary_col_size[S_COL_LOCKED], P_INT, NULL, NULL, NULL},

	/* Widget size */
	{"folderwin_x", "16", &prefs_common.folderwin_x, P_INT,
	 NULL, NULL, NULL},
	{"folderwin_y", "16", &prefs_common.folderwin_y, P_INT,
	 NULL, NULL, NULL},
	{"folderview_width", "179", &prefs_common.folderview_width, P_INT,
	 NULL, NULL, NULL},
	{"folderview_height", "450", &prefs_common.folderview_height, P_INT,
	 NULL, NULL, NULL},
	{"folderview_visible", "TRUE", &prefs_common.folderview_visible, P_BOOL,
	 NULL, NULL, NULL},

	{"folder_col_show_folder", "TRUE",
	 &prefs_common.folder_col_visible[F_COL_FOLDER], P_BOOL, NULL, NULL, NULL},
	{"folder_col_show_new", "TRUE",
	 &prefs_common.folder_col_visible[F_COL_NEW], P_BOOL, NULL, NULL, NULL},
	{"folder_col_show_unread", "TRUE",
	 &prefs_common.folder_col_visible[F_COL_UNREAD], P_BOOL, NULL, NULL, NULL},
	{"folder_col_show_total", "TRUE",
	 &prefs_common.folder_col_visible[F_COL_TOTAL], P_BOOL, NULL, NULL, NULL},

	{"folder_col_pos_folder", "0",
	 &prefs_common.folder_col_pos[F_COL_FOLDER], P_INT, NULL, NULL, NULL},
	{"folder_col_pos_new", "1",
	 &prefs_common.folder_col_pos[F_COL_NEW], P_INT, NULL, NULL, NULL},
	{"folder_col_pos_unread", "2",
	 &prefs_common.folder_col_pos[F_COL_UNREAD], P_INT, NULL, NULL, NULL},
	{"folder_col_pos_total", "3",
	 &prefs_common.folder_col_pos[F_COL_TOTAL], P_INT, NULL, NULL, NULL},

	{"folder_col_size_folder", "150",
	 &prefs_common.folder_col_size[F_COL_FOLDER], P_INT, NULL, NULL, NULL},
	{"folder_col_size_new", "32",
	 &prefs_common.folder_col_size[F_COL_NEW], P_INT, NULL, NULL, NULL},
	{"folder_col_size_unread", "32",
	 &prefs_common.folder_col_size[F_COL_UNREAD], P_INT, NULL, NULL, NULL},
	{"folder_col_size_total", "32",
	 &prefs_common.folder_col_size[F_COL_TOTAL], P_INT, NULL, NULL, NULL},

	{"summaryview_width", "600", &prefs_common.summaryview_width, P_INT,
	 NULL, NULL, NULL},
	{"summaryview_height", "157", &prefs_common.summaryview_height, P_INT,
	 NULL, NULL, NULL},

	{"main_messagewin_x", "256", &prefs_common.main_msgwin_x, P_INT,
	 NULL, NULL, NULL},
	{"main_messagewin_y", "210", &prefs_common.main_msgwin_y, P_INT,
	 NULL, NULL, NULL},
	{"messageview_width", "600", &prefs_common.msgview_width, P_INT,
	 NULL, NULL, NULL},
	{"messageview_height", "300", &prefs_common.msgview_height, P_INT,
	 NULL, NULL, NULL},
	{"messageview_visible", "TRUE", &prefs_common.msgview_visible, P_BOOL,
	 NULL, NULL, NULL},

	{"mainview_x", "64", &prefs_common.mainview_x, P_INT,
	 NULL, NULL, NULL},
	{"mainview_y", "64", &prefs_common.mainview_y, P_INT,
	 NULL, NULL, NULL},
	{"mainview_width", "600", &prefs_common.mainview_width, P_INT,
	 NULL, NULL, NULL},
	{"mainview_height", "600", &prefs_common.mainview_height, P_INT,
	 NULL, NULL, NULL},
	{"mainwin_x", "64", &prefs_common.mainwin_x, P_INT,
	 NULL, NULL, NULL},
	{"mainwin_y", "64", &prefs_common.mainwin_y, P_INT,
	 NULL, NULL, NULL},
	{"mainwin_width", "800", &prefs_common.mainwin_width, P_INT,
	 NULL, NULL, NULL},
	{"mainwin_height", "600", &prefs_common.mainwin_height, P_INT,
	 NULL, NULL, NULL},
	{"messagewin_width", "600", &prefs_common.msgwin_width, P_INT,
	 NULL, NULL, NULL},
	{"messagewin_height", "540", &prefs_common.msgwin_height, P_INT,
	 NULL, NULL, NULL},
	{"sourcewin_width", "600", &prefs_common.sourcewin_width, P_INT,
	 NULL, NULL, NULL},
	{"sourcewin_height", "500", &prefs_common.sourcewin_height, P_INT,
	 NULL, NULL, NULL},
	{"compose_width", "600", &prefs_common.compose_width, P_INT,
	 NULL, NULL, NULL},
	{"compose_height", "560", &prefs_common.compose_height, P_INT,
	 NULL, NULL, NULL},
	{"compose_x", "0", &prefs_common.compose_x, P_INT,
	 NULL, NULL, NULL},
	{"compose_y", "0", &prefs_common.compose_y, P_INT,
	 NULL, NULL, NULL},
	/* Message */
	{"enable_color", "TRUE", &prefs_common.enable_color, P_BOOL,
	 NULL, NULL, NULL},

	{"quote_level1_color", "179", &prefs_common.quote_level1_col, P_COLOR,
	 NULL, NULL, NULL},
	{"quote_level2_color", "179", &prefs_common.quote_level2_col, P_COLOR,
	 NULL, NULL, NULL},
	{"quote_level3_color", "179", &prefs_common.quote_level3_col, P_COLOR,
	 NULL, NULL, NULL},
	{"uri_color", "32512", &prefs_common.uri_col, P_COLOR,
	 NULL, NULL, NULL},
	{"target_folder_color", "14294218", &prefs_common.tgt_folder_col, P_COLOR,
	 NULL, NULL, NULL},
	{"signature_color", "7960953", &prefs_common.signature_col, P_COLOR,
	 NULL, NULL, NULL},
	{"recycle_quote_colors", "FALSE", &prefs_common.recycle_quote_colors,
	 P_BOOL, NULL, NULL, NULL},

	{"convert_mb_alnum", "FALSE", &prefs_common.conv_mb_alnum, P_BOOL,
	 NULL, NULL, NULL},
	{"display_header_pane", "TRUE", &prefs_common.display_header_pane,
	 P_BOOL, NULL, NULL, NULL},
	{"display_header", "TRUE", &prefs_common.display_header, P_BOOL,
	 NULL, NULL, NULL},
	{"render_html", "TRUE", &prefs_common.render_html, P_BOOL,
	 NULL, NULL, NULL},
	{"textview_cursor_visible", "FALSE",
	 &prefs_common.textview_cursor_visible, P_BOOL,
	 NULL, NULL, NULL},
	{"line_space", "2", &prefs_common.line_space, P_INT,
	 NULL, NULL, NULL},

	{"enable_smooth_scroll", "FALSE",
	 &prefs_common.enable_smooth_scroll, P_BOOL,
	 NULL, NULL, NULL},
	{"scroll_step", "1", &prefs_common.scroll_step, P_INT,
	 NULL, NULL, NULL},
	{"scroll_half_page", "FALSE", &prefs_common.scroll_halfpage, P_BOOL,
	 NULL, NULL, NULL},

	{"show_other_header", "FALSE", &prefs_common.show_other_header, P_BOOL,
	 NULL, NULL, NULL},

	{"attach_desc", "TRUE", &prefs_common.attach_desc, P_BOOL,
	 NULL, NULL, NULL},
	{"attach_save_directory", NULL,
	 &prefs_common.attach_save_dir, P_STRING, NULL, NULL, NULL},

	/* MIME viewer */
	{"mime_image_viewer", "display '%s'",
	 &prefs_common.mime_image_viewer, P_STRING, NULL, NULL, NULL},
	{"mime_audio_player", "play '%s'",
	 &prefs_common.mime_audio_player, P_STRING, NULL, NULL, NULL},
	{"mime_open_command", "gedit '%s'",
	 &prefs_common.mime_open_cmd, P_STRING, NULL, NULL, NULL},

	/* Interface */
	{"separate_folder", "FALSE", &prefs_common.sep_folder, P_BOOL,
	 NULL, NULL, NULL},
	{"separate_message", "FALSE", &prefs_common.sep_msg, P_BOOL,
	 NULL, NULL, NULL},

	/* {"emulate_emacs", "FALSE", &prefs_common.emulate_emacs, P_BOOL,
	 NULL, NULL, NULL}, */
	{"always_show_message_when_selected", "FALSE",
	 &prefs_common.always_show_msg,
	 P_BOOL, NULL, NULL, NULL},
	{"select_on_entry", NULL, &prefs_common.select_on_entry,
	 P_ENUM, NULL, NULL, NULL},
	{"mark_as_read_on_new_window", "FALSE",
	 &prefs_common.mark_as_read_on_new_window,
	 P_BOOL, NULL, NULL, NULL},
	{"mark_as_read_delay", "0",
	 &prefs_common.mark_as_read_delay, P_INT, 
	 NULL, NULL, NULL},
	{"open_inbox_on_inc", "FALSE", &prefs_common.open_inbox_on_inc,
	 P_BOOL, NULL, NULL, NULL},
	{"immediate_execution", "TRUE", &prefs_common.immediate_exec, P_BOOL,
	 NULL, NULL, NULL},
	{"nextunreadmsg_dialog", NULL, &prefs_common.next_unread_msg_dialog, P_ENUM,
	 NULL, NULL, NULL},

	{"pixmap_theme_path", DEFAULT_PIXMAP_THEME, 
	 &prefs_common.pixmap_theme_path, P_STRING,
	 NULL, NULL, NULL},

	{"hover_timeout", "500", &prefs_common.hover_timeout, P_INT,
	 NULL, NULL, NULL},
	
	/* Other */
	{"uri_open_command", DEFAULT_BROWSER_CMD,
	 &prefs_common.uri_cmd, P_STRING, NULL, NULL, NULL},
	{"print_command", "lpr %s", &prefs_common.print_cmd, P_STRING,
	 NULL, NULL, NULL},
	{"ext_editor_command", "gedit %s",
	 &prefs_common.ext_editor_cmd, P_STRING, NULL, NULL, NULL},

	{"add_address_by_click", "FALSE", &prefs_common.add_address_by_click,
	 P_BOOL, NULL, NULL, NULL},
	{"confirm_on_exit", "FALSE", &prefs_common.confirm_on_exit, P_BOOL,
	 NULL, NULL, NULL},
	{"clean_trash_on_exit", "FALSE", &prefs_common.clean_on_exit, P_BOOL,
	 NULL, NULL, NULL},
	{"ask_on_cleaning", "TRUE", &prefs_common.ask_on_clean, P_BOOL,
	 NULL, NULL, NULL},
	{"warn_queued_on_exit", "TRUE", &prefs_common.warn_queued_on_exit,
	 P_BOOL, NULL, NULL, NULL},
	{"work_offline", "FALSE", &prefs_common.work_offline, P_BOOL,
	 NULL, NULL, NULL},
	{"summary_quicksearch_type", "0", &prefs_common.summary_quicksearch_type, P_INT,
	 NULL, NULL, NULL},
	{"summary_quicksearch_sticky", "1", &prefs_common.summary_quicksearch_sticky, P_INT,
	 NULL, NULL, NULL},
	{"summary_quicksearch_recurse", "1", &prefs_common.summary_quicksearch_recurse, P_INT,
	 NULL, NULL, NULL},

	{"io_timeout_secs", "60", &prefs_common.io_timeout_secs,
	 P_INT, NULL, NULL, NULL},
	{"hide_score", "-9999", &prefs_common.kill_score, P_INT,
	 NULL, NULL, NULL},
	{"important_score", "1", &prefs_common.important_score, P_INT,
	 NULL, NULL, NULL},
        {"clip_log", "TRUE", &prefs_common.cliplog, P_BOOL,
	 NULL, NULL, NULL},
	{"log_length", "500", &prefs_common.loglength, P_INT,
	 NULL, NULL, NULL},

	{"cache_max_mem_usage", "4096", &prefs_common.cache_max_mem_usage, P_INT,
	 NULL, NULL, NULL},
	{"cache_min_keep_time", "15", &prefs_common.cache_min_keep_time, P_INT,
	 NULL, NULL, NULL},

	{"color_new", "179", &prefs_common.color_new, P_COLOR,
	 NULL, NULL, NULL},

	{"filteringwin_width", "500", &prefs_common.filteringwin_width, P_INT,
	 NULL, NULL, NULL},
	{"filteringwin_height", "-1", &prefs_common.filteringwin_height, P_INT,
	 NULL, NULL, NULL},
	{"warn_dnd", "1", &prefs_common.warn_dnd, P_INT,
	 NULL, NULL, NULL},

	{NULL, NULL, NULL, P_OTHER, NULL, NULL, NULL}
};

PrefsCommon *prefs_common_get(void)
{
	return &prefs_common;
}

/*
 * Read history list from the specified history file
 */
GList *prefs_common_read_history(const gchar *history) 
{
	FILE *fp;
	gchar *path;
	gchar buf[PREFSBUFSIZE];
	GList *tmp = NULL;

	path = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, history,
			   NULL);
	if ((fp = g_fopen(path, "rb")) == NULL) {
		if (ENOENT != errno) FILE_OP_ERROR(path, "fopen");
		g_free(path);
		return NULL;
	}
	g_free(path);
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		g_strstrip(buf);
		if (buf[0] == '\0') continue;
		tmp = add_history(tmp, buf);
	}
	fclose(fp);

	tmp = g_list_reverse(tmp);

	return tmp;
}

void prefs_common_read_config(void)
{
	gchar *rcpath;
	
	rcpath = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, COMMON_RC, NULL);
	prefs_read_config(param, "Common", rcpath, NULL);
	g_free(rcpath);
	
	prefs_common.mime_open_cmd_history =
		prefs_common_read_history(COMMAND_HISTORY);
	prefs_common.summary_quicksearch_history =
		prefs_common_read_history(QUICKSEARCH_HISTORY);
}

/*
 * Save history list to the specified history file
 */
void prefs_common_save_history(const gchar *history, GList *list)
{
	GList *cur;
	FILE *fp;
	gchar *path;

	path = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, history,
			   NULL);
	if ((fp = g_fopen(path, "wb")) == NULL) {
		FILE_OP_ERROR(path, "fopen");
		g_free(path);
		return;
	}

	for (cur = list; cur != NULL; cur = cur->next) {
		fputs((gchar *)cur->data, fp);
		fputc('\n', fp);
	}

	fclose(fp);
	g_free(path);
}

void prefs_common_write_config(void)
{
	prefs_write_config(param, "Common", COMMON_RC);

	prefs_common_save_history(COMMAND_HISTORY, 
		prefs_common.mime_open_cmd_history);
	prefs_common_save_history(QUICKSEARCH_HISTORY, 
		prefs_common.summary_quicksearch_history);
}

void pref_set_textview_from_pref(GtkTextView *textview, gchar *txt)
{
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(textview);
	gchar *o_out, *out = malloc(txt?(strlen(txt)+1):1);
	gchar *t = txt;
	memset(out, 0, strlen(txt)+1);
	o_out = out;
	while (*t != '\0') {
		if (*t == '\\' && *(t+1) == 'n') {
			*out++ = '\n';
			t++;
		} else if (*t == '\\') {
			t++;
		} else {
			*out++ = *t;
		}
		t++;
	}
	*out='\0';

	gtk_text_buffer_set_text(buffer, o_out?o_out:"", -1);
	g_free(o_out);
}

gchar *pref_get_pref_from_textview(GtkTextView *textview) 
{
	GtkTextBuffer *buffer;
	GtkTextIter start, end;
	gchar *o_out, *out, *tmp, *t;
	
	buffer = gtk_text_view_get_buffer(textview);
	gtk_text_buffer_get_start_iter(buffer, &start);
	gtk_text_buffer_get_iter_at_offset(buffer, &end, -1);
	tmp = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
	t = tmp;
	o_out = out = malloc(2*strlen(tmp)+1);
	
	while (*t != '\0') {
		if (*t == '\n') {
			*out++ = '\\';
			*out++ = 'n';
		} else if (*t == '\\') {
			*out++ = '\\';
			*out++ = '\\';
		} else {
			*out++ = *t;
		}
		t++;
	}
	*out = '\0';
	g_free(tmp);

	return o_out;
}
