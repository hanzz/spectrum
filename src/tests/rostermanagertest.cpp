#include "rostermanagertest.h"
#include "rostermanager.h"
#include "transport.h"


void RosterManagerTest::up (void) {
	m_buddy1 = new SpectrumBuddyTest(1, NULL);
	m_buddy1->setName("user1@example.com");
	m_buddy1->setAlias("Frank");
	m_buddy1->setOnline();
	
	m_buddy2 = new SpectrumBuddyTest(2, NULL);
	m_buddy2->setName("user2@example.com");
	m_buddy2->setAlias("Bob");
	m_buddy2->setOnline();
	
	m_user = new TestingUser("key", "user@example.com");
	m_manager = new SpectrumRosterManager(m_user);
}

void RosterManagerTest::down (void) {
	delete m_buddy1;
	delete m_buddy2;
	delete m_manager;
	delete m_user;
}

void RosterManagerTest::setRoster() {
	GHashTable *roster = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	g_hash_table_replace(roster, g_strdup(m_buddy1->getName().c_str()), m_buddy1);
	g_hash_table_replace(roster, g_strdup(m_buddy2->getName().c_str()), m_buddy2);
	m_manager->setRoster(roster);
	CPPUNIT_ASSERT(m_manager->getRosterItem(m_buddy1->getName()) == m_buddy1);
	CPPUNIT_ASSERT(m_manager->getRosterItem(m_buddy2->getName()) == m_buddy2);
	CPPUNIT_ASSERT(m_manager->getRosterItem("something") == NULL);
}

void RosterManagerTest::generatePresenceStanza() {
	setRoster();
	m_buddy1->setStatus(PURPLE_STATUS_AVAILABLE);
	m_buddy1->setStatusMessage("I'm here");
	m_buddy1->setIconHash("somehash");
	Tag *tag;
	
	std::string r;
	r =	"<presence from='user1%example.com@icq.localhost/bot'>"
			"<status>I'm here</status>"
			"<x xmlns='vcard-temp:x:update'>"
				"<photo>somehash</photo>"
			"</x>"
		"</presence>";
	tag = m_buddy1->generatePresenceStanza(TRANSPORT_FEATURE_AVATARS);
	compare(tag, r);

	r =	"<presence from='user1%example.com@icq.localhost/bot'>"
			"<status>I'm here</status>"
		"</presence>";
	tag = m_buddy1->generatePresenceStanza(0);
	compare(tag, r);

	r =	"<presence from='user1%example.com@icq.localhost/bot'>"
			"<show>away</show>"
			"<status>I'm here</status>"
		"</presence>";
	m_buddy1->setStatus(PURPLE_STATUS_AWAY);
	tag = m_buddy1->generatePresenceStanza(0);
	compare(tag, r);

	r =	"<presence from='user1%example.com@icq.localhost/bot'>"
			"<show>away</show>"
		"</presence>";
	m_buddy1->setStatusMessage("");
	tag = m_buddy1->generatePresenceStanza(0);
	compare(tag, r);
}


void RosterManagerTest::sendUnavailablePresenceToAll() {
	m_manager->sendUnavailablePresenceToAll();
	testTagCount(0);
	
	setRoster();
	m_manager->sendUnavailablePresenceToAll();
	testTagCount(2);

	std::list <std::string> users;
	users.push_back("user1%example.com@icq.localhost/bot");
	users.push_back("user2%example.com@icq.localhost/bot");

	CPPUNIT_ASSERT_MESSAGE ("Nothing was sent #1", m_tags.size() != 0);
	
	while (m_tags.size() != 0) {
		Tag *tag = m_tags.front();
		CPPUNIT_ASSERT (tag->name() == "presence");
		CPPUNIT_ASSERT (tag->findAttribute("type") == "unavailable");
		CPPUNIT_ASSERT (tag->findAttribute("to") == "user@example.com");
		CPPUNIT_ASSERT_MESSAGE ("Presence for user who is not in roster",
								find(users.begin(), users.end(), tag->findAttribute("from")) != users.end());
		users.remove(tag->findAttribute("from"));
		
		
		m_tags.remove(tag);
		delete tag;
	}
	CPPUNIT_ASSERT_MESSAGE ("Presence for one or more users from roster was not sent", users.size() == 0);
	
	m_buddy1->setOffline();
	m_buddy2->setOnline();
	Transport::instance()->clearTags();
	m_manager->sendUnavailablePresenceToAll();
	m_tags = Transport::instance()->getTags();

	CPPUNIT_ASSERT_MESSAGE ("Nothing was sent #2", m_tags.size() != 0);

	while (m_tags.size() != 0) {
		Tag *tag = m_tags.front();
		CPPUNIT_ASSERT (tag->name() == "presence");
		CPPUNIT_ASSERT (tag->findAttribute("type") == "unavailable");
		CPPUNIT_ASSERT (tag->findAttribute("to") == "user@example.com");
		CPPUNIT_ASSERT_MESSAGE ("Presence sent, but it's nof for m_buddy2",
								tag->findAttribute("from") == "user2%example.com@icq.localhost/bot");
		users.remove(tag->findAttribute("from"));

		m_tags.remove(tag);
		delete tag;
	}
}

void RosterManagerTest::sendPresenceToAll() {
	setRoster();
	m_buddy1->setStatus(PURPLE_STATUS_AVAILABLE);
	m_buddy1->setStatusMessage("I'm here");
	m_buddy1->setIconHash("somehash");
	m_buddy2->setStatus(PURPLE_STATUS_AVAILABLE);
	m_buddy2->setStatusMessage("I'm here");
	m_buddy2->setIconHash("somehash");
	m_manager->sendPresenceToAll("user@example.com");
	m_tags = Transport::instance()->getTags();

	std::list <std::string> users;
	users.push_back("user1%example.com@icq.localhost/bot");
	users.push_back("user2%example.com@icq.localhost/bot");

	CPPUNIT_ASSERT_MESSAGE ("Nothing was sent #1", m_tags.size() != 0);
	
	while (m_tags.size() != 0) {
		Tag *tag = m_tags.front();
		CPPUNIT_ASSERT (tag->name() == "presence");
		CPPUNIT_ASSERT (tag->findAttribute("type") == "");
		CPPUNIT_ASSERT (tag->findAttribute("to") == "user@example.com");
		CPPUNIT_ASSERT_MESSAGE ("Presence for user who is not in roster",
								find(users.begin(), users.end(), tag->findAttribute("from")) != users.end());
		users.remove(tag->findAttribute("from"));
		
		
		m_tags.remove(tag);
		delete tag;
	}
	CPPUNIT_ASSERT_MESSAGE ("Presence for one or more users from roster was not sent", users.size() == 0);
	Transport::instance()->clearTags();
}

void RosterManagerTest::isInRoster() {
	setRoster();
	CPPUNIT_ASSERT (m_manager->isInRoster("user1@example.com", ""));
	m_manager->removeFromLocalRoster("user1@example.com");
	CPPUNIT_ASSERT (!m_manager->isInRoster("user1@example.com", ""));
}

void RosterManagerTest::addRosterItem() {
	m_manager->addRosterItem(m_buddy1);
	m_manager->addRosterItem(m_buddy2);
	CPPUNIT_ASSERT (m_manager->isInRoster("user1@example.com", ""));
	CPPUNIT_ASSERT (m_manager->isInRoster("user2@example.com", ""));
}

void RosterManagerTest::sendPresence() {
	Tag *tag;
	m_buddy1->setStatus(PURPLE_STATUS_AVAILABLE);
	m_buddy1->setStatusMessage("I'm here");
	m_buddy1->setIconHash("somehash");
	m_manager->addRosterItem(m_buddy1);
	
	m_manager->sendPresence(m_buddy1, "resource");
	m_manager->sendPresence("user1@example.com", "resource");
	
	m_tags = Transport::instance()->getTags();
	CPPUNIT_ASSERT_MESSAGE ("There has to be 2 presence stanzas sent", m_tags.size() == 2);
	CPPUNIT_ASSERT (m_tags.back()->xml() == m_tags.front()->xml());
	delete m_tags.front();
	delete m_tags.back();
	m_tags.clear();
	Transport::instance()->clearTags();

	m_manager->sendPresence("user3@example.com", "resource");
	m_tags = Transport::instance()->getTags();
	CPPUNIT_ASSERT_MESSAGE ("There has to be 1 presence stanza sent", m_tags.size() == 1);

	tag = m_tags.front();
	CPPUNIT_ASSERT (tag->name() == "presence");
	CPPUNIT_ASSERT (tag->findAttribute("from") == "user3%example.com@icq.localhost");
	CPPUNIT_ASSERT (tag->findAttribute("type") == "unavailable");
	CPPUNIT_ASSERT (tag->findAttribute("to") == "user@example.com/resource");
	CPPUNIT_ASSERT (tag->findChild("x") == NULL);
	delete m_tags.front();
	m_tags.clear();
	Transport::instance()->clearTags();

	
	m_manager->sendPresence("user1@example.com", "resource");
	m_tags = Transport::instance()->getTags();
	CPPUNIT_ASSERT_MESSAGE ("There has to be 1 presence stanza sent", m_tags.size() == 1);

	tag = m_tags.front();
	CPPUNIT_ASSERT (tag->name() == "presence");
	CPPUNIT_ASSERT (tag->findAttribute("from") == "user1%example.com@icq.localhost/bot");
	CPPUNIT_ASSERT (tag->findChild("show") == NULL);
	CPPUNIT_ASSERT (tag->findChild("status") != NULL);
	CPPUNIT_ASSERT (tag->findChild("status")->cdata() == "I'm here");
	CPPUNIT_ASSERT (tag->findChild("x") == NULL);
	delete m_tags.front();
	m_tags.clear();
	Transport::instance()->clearTags();
}

void RosterManagerTest::handleBuddySignedOn() {
	Tag *tag;
	m_buddy1->setStatus(PURPLE_STATUS_AVAILABLE);
	m_buddy1->setStatusMessage("I'm here");
	m_buddy1->setIconHash("somehash");
	m_buddy1->setOffline();
	m_manager->addRosterItem(m_buddy1);

	m_manager->handleBuddySignedOn(m_buddy1);
	m_tags = Transport::instance()->getTags();

	CPPUNIT_ASSERT_MESSAGE ("There has to be 1 presence stanza sent", m_tags.size() == 1);

	tag = m_tags.front();
	CPPUNIT_ASSERT (tag->name() == "presence");
	CPPUNIT_ASSERT (tag->findAttribute("from") == "user1%example.com@icq.localhost/bot");
	CPPUNIT_ASSERT (tag->findChild("show") == NULL);
	CPPUNIT_ASSERT (tag->findChild("status") != NULL);
	CPPUNIT_ASSERT (tag->findChild("status")->cdata() == "I'm here");
	CPPUNIT_ASSERT (tag->findChild("x") == NULL);
	delete m_tags.front();
	m_tags.clear();
	Transport::instance()->clearTags();
	
	CPPUNIT_ASSERT (m_buddy1->isOnline());
}

void RosterManagerTest::handleBuddySignedOff() {
	Tag *tag;
	m_buddy1->setStatus(PURPLE_STATUS_OFFLINE);
	m_buddy1->setOnline();
	m_manager->addRosterItem(m_buddy1);

	m_manager->handleBuddySignedOff(m_buddy1);
	m_tags = Transport::instance()->getTags();

	CPPUNIT_ASSERT_MESSAGE ("There has to be 1 presence stanza sent", m_tags.size() == 1);

	tag = m_tags.front();
	CPPUNIT_ASSERT (tag->name() == "presence");
	CPPUNIT_ASSERT (tag->findAttribute("from") == "user1%example.com@icq.localhost/bot");
	CPPUNIT_ASSERT (tag->findAttribute("type") == "unavailable");
	CPPUNIT_ASSERT (tag->findAttribute("to") == "user@example.com");

	delete m_tags.front();
	m_tags.clear();
	Transport::instance()->clearTags();
	
	CPPUNIT_ASSERT (!m_buddy1->isOnline());
}

void RosterManagerTest::handleBuddyCreatedRIE() {
	m_manager->handleBuddyCreated(m_buddy1);
	CPPUNIT_ASSERT(Transport::instance()->getTags().empty());

	m_manager->syncBuddiesCallback();
	CPPUNIT_ASSERT(Transport::instance()->getTags().empty());

	m_manager->handleBuddyCreated(m_buddy2);

	m_manager->syncBuddiesCallback();
	CPPUNIT_ASSERT(Transport::instance()->getTags().empty());
	
	m_manager->syncBuddiesCallback();
	m_tags = Transport::instance()->getTags();

	CPPUNIT_ASSERT_MESSAGE ("There has to be 1 RIE stanza sent", m_tags.size() == 1);

	Tag *tag = m_tags.front();
	CPPUNIT_ASSERT (tag->name() == "iq");
	CPPUNIT_ASSERT (tag->findAttribute("to") == "user@example.com/psi");
	CPPUNIT_ASSERT (tag->findAttribute("type") == "set");
	CPPUNIT_ASSERT (tag->findAttribute("from") == "icq.localhost");
	CPPUNIT_ASSERT (tag->findChild("x") != NULL);
	CPPUNIT_ASSERT (tag->findChild("x")->findAttribute("xmlns") == "http://jabber.org/protocol/rosterx");
	CPPUNIT_ASSERT (tag->findChild("x")->children().size() != 0);
	for (std::list<Tag *>::const_iterator i = tag->findChild("x")->children().begin(); i != tag->findChild("x")->children().end(); i++) {
		Tag *item = *i;
		CPPUNIT_ASSERT (item->name() == "item");
		CPPUNIT_ASSERT (item->findAttribute("jid") == "user1%example.com@icq.localhost" ||
						item->findAttribute("jid") == "user2%example.com@icq.localhost");
		if (item->findAttribute("jid") == "user1%example.com@icq.localhost")
			CPPUNIT_ASSERT (item->findAttribute("name") == "Frank");
		else
			CPPUNIT_ASSERT (item->findAttribute("name") == "Bob");
		CPPUNIT_ASSERT (item->findChild("group") != NULL);
		CPPUNIT_ASSERT (item->findChild("group")->cdata() == "Buddies");
	}

	delete m_tags.front();
	m_tags.clear();
	Transport::instance()->clearTags();
	
	CPPUNIT_ASSERT (m_manager->isInRoster("user1@example.com", ""));
	CPPUNIT_ASSERT (m_manager->isInRoster("user2@example.com", ""));
	
}

void RosterManagerTest::handleBuddyCreatedSubscribe() {
	m_user->setResource("psi", 10, 0); // no features => it should send subscribe instead of RIE.
	m_manager->handleBuddyCreated(m_buddy1);
	CPPUNIT_ASSERT(Transport::instance()->getTags().empty());

	m_manager->syncBuddiesCallback();
	CPPUNIT_ASSERT(Transport::instance()->getTags().empty());

	m_manager->handleBuddyCreated(m_buddy2);

	m_manager->syncBuddiesCallback();
	CPPUNIT_ASSERT(Transport::instance()->getTags().empty());
	
	m_manager->syncBuddiesCallback();
	m_tags = Transport::instance()->getTags();

	CPPUNIT_ASSERT_MESSAGE ("There has to be 2 subscribe stanza sent", m_tags.size() == 2);

	std::list <std::string> users;
	users.push_back("user1%example.com@icq.localhost");
	users.push_back("user2%example.com@icq.localhost");

	while (m_tags.size() != 0) {
		Tag *tag = m_tags.front();
		// <presence type='subscribe' from='user1%example.com@icq.localhost/bot' to='icq.localhost'>
		//   <nick xmlns='http://jabber.org/protocol/nick'>Frank</nick>
		// </presence>
		CPPUNIT_ASSERT (tag->name() == "presence");
		CPPUNIT_ASSERT (tag->findAttribute("type") == "subscribe");
		CPPUNIT_ASSERT (tag->findAttribute("to") == "user@example.com");
		CPPUNIT_ASSERT_MESSAGE ("Presence for user who is not in roster",
								find(users.begin(), users.end(), tag->findAttribute("from")) != users.end());
		CPPUNIT_ASSERT (tag->findChild("nick") != NULL);
		CPPUNIT_ASSERT (tag->findChild("nick")->findAttribute("xmlns") == "http://jabber.org/protocol/nick");
		if (tag->findAttribute("from") == "user1%example.com@icq.localhost")
			CPPUNIT_ASSERT (tag->findChild("nick")->cdata() == "Frank");
		else
			CPPUNIT_ASSERT (tag->findChild("nick")->cdata() == "Bob");
		users.remove(tag->findAttribute("from"));
		m_tags.remove(tag);
		delete tag;
	}
	CPPUNIT_ASSERT_MESSAGE ("Presence for one or more users from roster was not sent", users.size() == 0);
	Transport::instance()->clearTags();
	
	CPPUNIT_ASSERT (m_manager->isInRoster("user1@example.com", ""));
	CPPUNIT_ASSERT (m_manager->isInRoster("user2@example.com", ""));
	
}

void RosterManagerTest::handleBuddyCreatedRemove() {
	m_manager->handleBuddyCreated(m_buddy1);
	CPPUNIT_ASSERT(Transport::instance()->getTags().empty());

	m_manager->syncBuddiesCallback();
	CPPUNIT_ASSERT(Transport::instance()->getTags().empty());

	m_manager->handleBuddyCreated(m_buddy2);

	m_manager->syncBuddiesCallback();
	CPPUNIT_ASSERT(Transport::instance()->getTags().empty());
	
	m_manager->handleBuddyRemoved(m_buddy1);
	
	m_manager->syncBuddiesCallback();
	CPPUNIT_ASSERT(Transport::instance()->getTags().empty());

	m_manager->syncBuddiesCallback();
	m_tags = Transport::instance()->getTags();

	CPPUNIT_ASSERT_MESSAGE ("There has to be 1 RIE stanza sent", m_tags.size() == 1);

	Tag *tag = m_tags.front();
	CPPUNIT_ASSERT (tag->name() == "iq");
	CPPUNIT_ASSERT (tag->findAttribute("to") == "user@example.com/psi");
	CPPUNIT_ASSERT (tag->findAttribute("type") == "set");
	CPPUNIT_ASSERT (tag->findAttribute("from") == "icq.localhost");
	CPPUNIT_ASSERT (tag->findChild("x") != NULL);
	CPPUNIT_ASSERT (tag->findChild("x")->findAttribute("xmlns") == "http://jabber.org/protocol/rosterx");
	CPPUNIT_ASSERT (tag->findChild("x")->children().size() != 0);
	for (std::list<Tag *>::const_iterator i = tag->findChild("x")->children().begin(); i != tag->findChild("x")->children().end(); i++) {
		Tag *item = *i;
		CPPUNIT_ASSERT (item->name() == "item");
		CPPUNIT_ASSERT (item->findAttribute("jid") == "user2%example.com@icq.localhost");
		CPPUNIT_ASSERT (item->findAttribute("name") == "Bob");
		CPPUNIT_ASSERT (item->findChild("group") != NULL);
		CPPUNIT_ASSERT (item->findChild("group")->cdata() == "Buddies");
	}

	delete m_tags.front();
	m_tags.clear();
	Transport::instance()->clearTags();
	
	CPPUNIT_ASSERT (m_manager->isInRoster("user2@example.com", ""));
	
}

void RosterManagerTest::handleSubscriptionSubscribe() {
	Subscription s(Subscription::Subscribe, JID("user1%example.com@icq.localhost"));
}
