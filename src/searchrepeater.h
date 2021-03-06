/**
 * XMPP - libpurple transport
 *
 * Copyright (C) 2009, Jan Kaluza <hanzz@soc.pidgin.im>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 */

#ifndef _HI_SEARCH_REPEATER_H
#define _HI_SEARCH_REPEATER_H

#include <string>
#include "purple.h"
#include "account.h"
#include "glib.h"
#include "request.h"
#include "abstractpurplerequest.h"

class GlooxMessageHandler;
class User;

class SearchRepeater : public AbstractPurpleRequest
{
	public:
		// Request input
		SearchRepeater(GlooxMessageHandler *m, User *user, const std::string &title, const std::string &primaryString, const std::string &secondaryString, const std::string &value, gboolean multiline, gboolean masked, GCallback ok_cb, GCallback cancel_cb, void * user_data);
		// Request fields
		SearchRepeater(GlooxMessageHandler *m, User *user, const std::string &title, const std::string &primaryString, const std::string &secondaryString, PurpleRequestFields *fields, GCallback ok_cb, GCallback cancel_cb, void * user_data);
		~SearchRepeater();
		bool handleIq(const IQ &iq);
		void sendSearchResults(PurpleNotifySearchResults *results);
		void setType(PurpleRequestType t) { m_type = t; }
		PurpleRequestType type() { return m_type; }
		const std::string & getInitiator() { return m_from; }

	private:
		GlooxMessageHandler *main;
		User *m_user;
		Tag *m_lastTag;
		void *m_requestData;
		GCallback m_ok_cb;
		GCallback m_cancel_cb;
		PurpleRequestType m_type;
		std::string m_from;
		std::map<int, GCallback> m_actions;
		PurpleRequestFields *m_fields;
};

#endif

