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

/*
 * Edit address item data.
 */

#ifndef __EDITADDRESS_H__
#define __EDITADDRESS_H__

typedef void (*EditAddressPostUpdateCallback) (ItemPerson *person);

ItemPerson *addressbook_edit_person( AddressBookFile *abf, ItemFolder *parent, ItemPerson *person,
									 gboolean pgMail, GtkWidget *parent_container,
									 EditAddressPostUpdateCallback,
									 gboolean get_focus );
void addressbook_edit_person_invalidate( AddressBookFile *abf, ItemFolder *parent_folder,
										 ItemPerson *person );
void addressbook_edit_person_widgetset_hide( void );
void addressbook_edit_reload_attr_list( void );

#endif /* __EDITADDRESS_H__ */
