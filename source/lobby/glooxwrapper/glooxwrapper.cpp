/* Copyright (C) 2015 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "glooxwrapper.h"

#include <gloox/connectionlistener.h>
#include <gloox/error.h>
#include <gloox/glooxversion.h>
#include <gloox/messagehandler.h>

#if OS_WIN
const std::string gloox::EmptyString = "";
#endif

void* glooxwrapper::glooxwrapper_alloc(size_t size)
{
	void* p = malloc(size);
	if (p == NULL)
		throw std::bad_alloc();
	return p;
}

void glooxwrapper::glooxwrapper_free(void* p)
{
	free(p);
}


namespace glooxwrapper
{

class ConnectionListenerWrapper : public gloox::ConnectionListener
{
	glooxwrapper::ConnectionListener* m_Wrapped;
public:
	ConnectionListenerWrapper(glooxwrapper::ConnectionListener* wrapped) : m_Wrapped(wrapped) {}

	virtual void onConnect()
	{
		m_Wrapped->onConnect();
	}

	virtual void onDisconnect(gloox::ConnectionError e)
	{
		m_Wrapped->onDisconnect(e);
	}

	virtual bool onTLSConnect(const gloox::CertInfo& info)
	{
		glooxwrapper::CertInfo infoWrapped;
#define COPY(n) infoWrapped.n = info.n
		COPY(status);
		COPY(chain);
		COPY(issuer);
		COPY(server);
		COPY(date_from);
		COPY(date_to);
		COPY(protocol);
		COPY(cipher);
		COPY(mac);
		COPY(compression);
#undef COPY
		return m_Wrapped->onTLSConnect(infoWrapped);
	}
};

class IqHandlerWrapper : public gloox::IqHandler
{
	glooxwrapper::IqHandler* m_Wrapped;
public:
	IqHandlerWrapper(glooxwrapper::IqHandler* wrapped) : m_Wrapped(wrapped) {}

	virtual bool handleIq(const gloox::IQ& iq)
	{
		glooxwrapper::IQ iqWrapper(iq);
		return m_Wrapped->handleIq(iqWrapper);
	}

	virtual void handleIqID(const gloox::IQ& iq, int context)
	{
		glooxwrapper::IQ iqWrapper(iq);
		m_Wrapped->handleIqID(iqWrapper, context);
	}
};

class MessageHandlerWrapper : public gloox::MessageHandler
{
	glooxwrapper::MessageHandler* m_Wrapped;
public:
	MessageHandlerWrapper(glooxwrapper::MessageHandler* wrapped) : m_Wrapped(wrapped) {}

	virtual void handleMessage(const gloox::Message& msg, gloox::MessageSession* UNUSED(session))
	{
		/* MessageSession not supported */
		glooxwrapper::Message msgWrapper(const_cast<gloox::Message*>(&msg), false);
		m_Wrapped->handleMessage(msgWrapper, NULL);
	}
};

class MUCRoomHandlerWrapper : public gloox::MUCRoomHandler
{
	glooxwrapper::MUCRoomHandler* m_Wrapped;
public:
	MUCRoomHandlerWrapper(glooxwrapper::MUCRoomHandler* wrapped) : m_Wrapped(wrapped) {}

	virtual ~MUCRoomHandlerWrapper() {}

	virtual void handleMUCParticipantPresence(gloox::MUCRoom* UNUSED(room), const gloox::MUCRoomParticipant participant, const gloox::Presence& presence)
	{
		glooxwrapper::MUCRoomParticipant part;
		glooxwrapper::JID nick(*participant.nick);
		glooxwrapper::JID jid(*participant.jid);
		glooxwrapper::JID actor(*participant.actor);
		glooxwrapper::JID alternate(*participant.alternate);
		part.nick = participant.nick ? &nick : NULL;
		part.affiliation = participant.affiliation;
		part.role = participant.role;
		part.jid = participant.jid ? &jid : NULL;
		part.flags = participant.flags;
		part.reason = participant.reason;
		part.actor = participant.actor ? &actor : NULL;
		part.newNick = participant.newNick;
		part.status = participant.status;
		part.alternate = participant.alternate ? &alternate : NULL;

		/* MUCRoom not supported */
		m_Wrapped->handleMUCParticipantPresence(NULL, part, glooxwrapper::Presence(presence.presence()));

		/* gloox 1.0 leaks some JIDs (fixed in 1.0.1), so clean them up */
#if GLOOXVERSION == 0x10000
		delete participant.jid;
		delete participant.actor;
		delete participant.alternate;
#endif
	}

	virtual void handleMUCMessage(gloox::MUCRoom* UNUSED(room), const gloox::Message& msg, bool priv)
	{
		glooxwrapper::Message msgWrapper(const_cast<gloox::Message*>(&msg), false);

		/* MUCRoom not supported */
		m_Wrapped->handleMUCMessage(NULL, msgWrapper, priv);
	}

	virtual bool handleMUCRoomCreation(gloox::MUCRoom* UNUSED(room))
	{
		/* Not supported */
		return false;
	}

	virtual void handleMUCSubject(gloox::MUCRoom* UNUSED(room), const std::string& nick, const std::string& subject)
	{
		/* MUCRoom not supported */
		m_Wrapped->handleMUCSubject(NULL, nick, subject);
	}

	virtual void handleMUCInviteDecline(gloox::MUCRoom* UNUSED(room), const gloox::JID& UNUSED(invitee), const std::string& UNUSED(reason))
	{
		/* Not supported */
	}

	virtual void handleMUCError(gloox::MUCRoom* UNUSED(room), gloox::StanzaError error)
	{
		/* MUCRoom not supported */
		m_Wrapped->handleMUCError(NULL, error);
	}

	virtual void handleMUCInfo(gloox::MUCRoom* UNUSED(room), int UNUSED(features), const std::string& UNUSED(name), const gloox::DataForm* UNUSED(infoForm))
	{
		/* Not supported */
	}

	virtual void handleMUCItems(gloox::MUCRoom* UNUSED(room), const gloox::Disco::ItemList& UNUSED(items))
	{
		/* Not supported */
	}
};

class RegistrationHandlerWrapper : public gloox::RegistrationHandler
{
	glooxwrapper::RegistrationHandler* m_Wrapped;
public:
	RegistrationHandlerWrapper(glooxwrapper::RegistrationHandler* wrapped) : m_Wrapped(wrapped) {}

	virtual void handleRegistrationFields(const gloox::JID& from, int fields, std::string instructions)
	{
		glooxwrapper::JID fromWrapped(from);
		m_Wrapped->handleRegistrationFields(fromWrapped, fields, instructions);
	}

	virtual void handleAlreadyRegistered(const gloox::JID& from)
	{
		glooxwrapper::JID fromWrapped(from);
		m_Wrapped->handleAlreadyRegistered(fromWrapped);
	}

	virtual void handleRegistrationResult(const gloox::JID& from, gloox::RegistrationResult regResult)
	{
		glooxwrapper::JID fromWrapped(from);
		m_Wrapped->handleRegistrationResult(fromWrapped, regResult);
	}

	virtual void handleDataForm(const gloox::JID& from, const gloox::DataForm& UNUSED(form))
	{
		glooxwrapper::JID fromWrapped(from);
		/* DataForm not supported */
		m_Wrapped->handleDataForm(fromWrapped, *(glooxwrapper::DataForm*)NULL);
	}

	virtual void handleOOB(const gloox::JID& from, const gloox::OOB& UNUSED(oob))
	{
		glooxwrapper::JID fromWrapped(from);
		/* OOB not supported */
		m_Wrapped->handleOOB(fromWrapped, *(glooxwrapper::OOB*)NULL);
	}
};

class StanzaExtensionWrapper : public gloox::StanzaExtension
{
public:
	const glooxwrapper::StanzaExtension* m_Wrapped;
	bool m_Owned;
	std::string m_FilterString;

	StanzaExtensionWrapper(const glooxwrapper::StanzaExtension* wrapped, bool owned) :
	gloox::StanzaExtension(wrapped->extensionType()), m_Wrapped(wrapped), m_Owned(owned)
	{
		m_FilterString = m_Wrapped->filterString().to_string();
	}

	~StanzaExtensionWrapper()
	{
		if (m_Owned)
			delete m_Wrapped;
	}

	virtual const std::string& filterString() const
	{
		return m_FilterString;
	}

	virtual gloox::StanzaExtension* newInstance(const gloox::Tag* tag) const
	{
		glooxwrapper::Tag* tagWrapper = glooxwrapper::Tag::allocate(const_cast<gloox::Tag*>(tag), false);
		gloox::StanzaExtension* ret = new StanzaExtensionWrapper(m_Wrapped->newInstance(tagWrapper), true);
		glooxwrapper::Tag::free(tagWrapper);
		return ret;
	}

	virtual gloox::Tag* tag() const
	{
		glooxwrapper::Tag* wrapper = m_Wrapped->tag();
		gloox::Tag* ret = wrapper->stealWrapped();
		glooxwrapper::Tag::free(wrapper);
		return ret;
	}

	virtual gloox::StanzaExtension* clone() const
	{
		return new StanzaExtensionWrapper(m_Wrapped->clone(), true);
	}
};

class ClientImpl
{
public:
	// List of registered callback wrappers, to get deleted when Client is deleted
	std::list<shared_ptr<gloox::ConnectionListener> > m_ConnectionListeners;
	std::list<shared_ptr<gloox::MessageHandler> > m_MessageHandlers;
	std::list<shared_ptr<gloox::IqHandler> > m_IqHandlers;
};

} // namespace glooxwrapper


glooxwrapper::Client::Client(const string& server)
{
	m_Wrapped = new gloox::Client(server.to_string());
	m_DiscoWrapper = new glooxwrapper::Disco(m_Wrapped->disco());
	m_Impl = new ClientImpl;
}

glooxwrapper::Client::Client(const JID& jid, const string& password, int port)
{
	m_Wrapped = new gloox::Client(jid.getWrapped(), password.to_string(), port);
	m_DiscoWrapper = new glooxwrapper::Disco(m_Wrapped->disco());
	m_Impl = new ClientImpl;
}

glooxwrapper::Client::~Client()
{
	delete m_Wrapped;
	delete m_DiscoWrapper;
	delete m_Impl;
}

bool glooxwrapper::Client::connect(bool block)
{
	return m_Wrapped->connect(block);
}

gloox::ConnectionError glooxwrapper::Client::recv(int timeout)
{
	return m_Wrapped->recv(timeout);
}

void glooxwrapper::Client::send(const IQ& iq)
{
	m_Wrapped->send(iq.getWrapped());
}

void glooxwrapper::Client::setTls(gloox::TLSPolicy tls)
{
	m_Wrapped->setTls(tls);
}

void glooxwrapper::Client::setCompression(bool compression)
{
	m_Wrapped->setCompression(compression);
}

void glooxwrapper::Client::setSASLMechanisms(int mechanisms)
{
	m_Wrapped->setSASLMechanisms(mechanisms);
}

void glooxwrapper::Client::disconnect()
{
	m_Wrapped->disconnect();
}

void glooxwrapper::Client::registerStanzaExtension(glooxwrapper::StanzaExtension* ext)
{
	gloox::StanzaExtension* stanza = new StanzaExtensionWrapper(ext, true);
	m_Wrapped->registerStanzaExtension(stanza);
}

void glooxwrapper::Client::registerConnectionListener(glooxwrapper::ConnectionListener* hnd)
{
	gloox::ConnectionListener* listener = new ConnectionListenerWrapper(hnd);
	m_Wrapped->registerConnectionListener(listener);
	m_Impl->m_ConnectionListeners.push_back(shared_ptr<gloox::ConnectionListener>(listener));
}

void glooxwrapper::Client::registerMessageHandler(glooxwrapper::MessageHandler* hnd)
{
	gloox::MessageHandler* handler = new MessageHandlerWrapper(hnd);
	m_Wrapped->registerMessageHandler(handler);
	m_Impl->m_MessageHandlers.push_back(shared_ptr<gloox::MessageHandler>(handler));
}

void glooxwrapper::Client::registerIqHandler(glooxwrapper::IqHandler* ih, int exttype)
{
	gloox::IqHandler* handler = new IqHandlerWrapper(ih);
	m_Wrapped->registerIqHandler(handler, exttype);
	m_Impl->m_IqHandlers.push_back(shared_ptr<gloox::IqHandler>(handler));
}

bool glooxwrapper::Client::removePresenceExtension(int type)
{
	return m_Wrapped->removePresenceExtension(type);
}

void glooxwrapper::Client::setPresence(gloox::Presence::PresenceType pres, int priority, const string& status)
{
	m_Wrapped->setPresence(pres, priority, status.to_string());
}


glooxwrapper::DelayedDelivery::DelayedDelivery(const gloox::DelayedDelivery* wrapped)
{
	m_Wrapped = wrapped;
}

const glooxwrapper::string glooxwrapper::DelayedDelivery::stamp() const
{
	return m_Wrapped->stamp();
}


glooxwrapper::Disco::Disco(gloox::Disco* wrapped)
{
	m_Wrapped = wrapped;
}

void glooxwrapper::Disco::setVersion(const string& name, const string& version, const string& os)
{
	m_Wrapped->setVersion(name.to_string(), version.to_string(), os.to_string());
}

void glooxwrapper::Disco::setIdentity(const string& category, const string& type, const string& name)
{
	m_Wrapped->setIdentity(category.to_string(), type.to_string(), name.to_string());
}


glooxwrapper::IQ::IQ(gloox::IQ::IqType type, const JID& to, const string& id)
{
	m_Wrapped = new gloox::IQ(type, to.getWrapped(), id.to_string());
	m_Owned = true;
}

glooxwrapper::IQ::~IQ()
{
	if (m_Owned)
		delete m_Wrapped;
}

void glooxwrapper::IQ::addExtension(const glooxwrapper::StanzaExtension* se)
{
	m_Wrapped->addExtension(new StanzaExtensionWrapper(se, true));
}

const glooxwrapper::StanzaExtension* glooxwrapper::IQ::findExtension(int type) const
{
	const gloox::StanzaExtension* ext = m_Wrapped->findExtension(type);
	if (!ext)
		return NULL;
	return static_cast<const StanzaExtensionWrapper*>(ext)->m_Wrapped;
}

gloox::StanzaError glooxwrapper::IQ::error_error() const
{
	return m_Wrapped->error()->error();
}

glooxwrapper::Tag* glooxwrapper::IQ::tag() const
{
	return Tag::allocate(m_Wrapped->tag(), true);
}

gloox::IQ::IqType glooxwrapper::IQ::subtype() const
{
	return m_Wrapped->subtype();
}


glooxwrapper::JID::JID()
{
	m_Wrapped = new gloox::JID();
	m_Owned = true;
}

glooxwrapper::JID::JID(const glooxwrapper::string& jid)
{
	m_Wrapped = new gloox::JID(jid.to_string());
	m_Owned = true;
}

void glooxwrapper::JID::init(const char* data, size_t len)
{
	m_Wrapped = new gloox::JID(std::string(data, data+len));
	m_Owned = true;
}

glooxwrapper::JID::~JID()
{
	if (m_Owned)
		delete m_Wrapped;
}

glooxwrapper::string glooxwrapper::JID::username() const
{
	return m_Wrapped->username();
}

glooxwrapper::string glooxwrapper::JID::resource() const
{
	return m_Wrapped->resource();
};


glooxwrapper::Message::Message(gloox::Message* wrapped, bool owned)
	: m_Wrapped(wrapped), m_Owned(owned)
{
	m_From = new glooxwrapper::JID(m_Wrapped->from());
}

glooxwrapper::Message::~Message()
{
	if (m_Owned)
		delete m_Wrapped;
	delete m_From;
}

gloox::Message::MessageType glooxwrapper::Message::subtype() const
{
	return m_Wrapped->subtype();
}

const glooxwrapper::JID& glooxwrapper::Message::from() const
{
	return *m_From;
}

glooxwrapper::string glooxwrapper::Message::body() const
{
	return m_Wrapped->body();
}

glooxwrapper::string glooxwrapper::Message::subject(const string& lang) const
{
	return m_Wrapped->subject(lang.to_string());
}

glooxwrapper::string glooxwrapper::Message::thread() const
{
	return m_Wrapped->thread();
}

const glooxwrapper::DelayedDelivery* glooxwrapper::Message::when() const
{
	const gloox::DelayedDelivery* wrapped = m_Wrapped->when();
	if (wrapped == 0)
		return 0;
	return new glooxwrapper::DelayedDelivery(wrapped);
}


glooxwrapper::MUCRoom::MUCRoom(Client* parent, const JID& nick, MUCRoomHandler* mrh, MUCRoomConfigHandler* UNUSED(mrch))
{
	m_HandlerWrapper = new MUCRoomHandlerWrapper(mrh);
	m_Wrapped = new gloox::MUCRoom(parent ? parent->getWrapped() : NULL, nick.getWrapped(), m_HandlerWrapper);
}

glooxwrapper::MUCRoom::~MUCRoom()
{
	delete m_Wrapped;
	delete m_HandlerWrapper;
}

const glooxwrapper::string glooxwrapper::MUCRoom::nick() const
{
	return m_Wrapped->nick();
}

void glooxwrapper::MUCRoom::join(gloox::Presence::PresenceType type, const string& status, int priority)
{
	m_Wrapped->join(type, status.to_string(), priority);
}

void glooxwrapper::MUCRoom::leave(const string& msg)
{
	m_Wrapped->leave(msg.to_string());
}

void glooxwrapper::MUCRoom::send(const string& message)
{
	m_Wrapped->send(message.to_string());
}

void glooxwrapper::MUCRoom::setNick(const string& nick)
{
	m_Wrapped->setNick(nick.to_string());
}

void glooxwrapper::MUCRoom::setPresence(gloox::Presence::PresenceType presence, const string& msg)
{
	m_Wrapped->setPresence(presence, msg.to_string());
}

void glooxwrapper::MUCRoom::setRequestHistory(int value, gloox::MUCRoom::HistoryRequestType type)
{
	m_Wrapped->setRequestHistory(value, type);
}

void glooxwrapper::MUCRoom::kick(const string& nick, const string& reason)
{
	m_Wrapped->kick(nick.to_string(), reason.to_string());
}

void glooxwrapper::MUCRoom::ban(const string& nick, const string& reason)
{
	m_Wrapped->ban(nick.to_string(), reason.to_string());
}


glooxwrapper::Registration::Registration(Client* parent)
{
	m_Wrapped = new gloox::Registration(parent->getWrapped());
}

glooxwrapper::Registration::~Registration()
{
	delete m_Wrapped;
}

void glooxwrapper::Registration::fetchRegistrationFields()
{
	m_Wrapped->fetchRegistrationFields();
}

bool glooxwrapper::Registration::createAccount(int fields, const glooxwrapper::RegistrationFields& values)
{
	gloox::RegistrationFields valuesUnwrapped;
#define COPY(n) valuesUnwrapped.n = values.n.to_string()
	COPY(username);
	COPY(nick);
	COPY(password);
	COPY(name);
	COPY(first);
	COPY(last);
	COPY(email);
	COPY(address);
	COPY(city);
	COPY(state);
	COPY(zip);
	COPY(phone);
	COPY(url);
	COPY(date);
	COPY(misc);
	COPY(text);
#undef COPY
	return m_Wrapped->createAccount(fields, valuesUnwrapped);
}

void glooxwrapper::Registration::registerRegistrationHandler(RegistrationHandler* rh)
{
	gloox::RegistrationHandler* handler = new RegistrationHandlerWrapper(rh);
	m_Wrapped->registerRegistrationHandler(handler);
	m_RegistrationHandlers.push_back(shared_ptr<gloox::RegistrationHandler>(handler));
}


glooxwrapper::Tag::Tag(const string& name)
{
	m_Wrapped = new gloox::Tag(name.to_string());
	m_Owned = true;
}

glooxwrapper::Tag::Tag(const string& name, const string& cdata)
{
	m_Wrapped = new gloox::Tag(name.to_string(), cdata.to_string());
	m_Owned = true;
}

glooxwrapper::Tag::~Tag()
{
	if (m_Owned)
		delete m_Wrapped;
}

glooxwrapper::Tag* glooxwrapper::Tag::allocate(const string& name)
{
	return new glooxwrapper::Tag(name);
}

glooxwrapper::Tag* glooxwrapper::Tag::allocate(const string& name, const string& cdata)
{
	return new glooxwrapper::Tag(name, cdata);
}

glooxwrapper::Tag* glooxwrapper::Tag::allocate(gloox::Tag* wrapped, bool owned)
{
	return new glooxwrapper::Tag(wrapped, owned);
}

void glooxwrapper::Tag::free(const glooxwrapper::Tag* tag)
{
	delete tag;
}

bool glooxwrapper::Tag::addAttribute(const string& name, const string& value)
{
	return m_Wrapped->addAttribute(name.to_string(), value.to_string());
}

glooxwrapper::string glooxwrapper::Tag::findAttribute(const string& name) const
{
	return m_Wrapped->findAttribute(name.to_string());
}

glooxwrapper::Tag* glooxwrapper::Tag::clone() const
{
	return new glooxwrapper::Tag(m_Wrapped->clone(), true);
}

glooxwrapper::string glooxwrapper::Tag::xmlns() const
{
	return m_Wrapped->xmlns();
}

bool glooxwrapper::Tag::setXmlns(const string& xmlns)
{
	return m_Wrapped->setXmlns(xmlns.to_string());
}

glooxwrapper::string glooxwrapper::Tag::xml() const
{
	return m_Wrapped->xml();
}

void glooxwrapper::Tag::addChild(Tag* child)
{
	m_Wrapped->addChild(child->stealWrapped());
	Tag::free(child);
}

glooxwrapper::string glooxwrapper::Tag::name() const
{
	return m_Wrapped->name();
}

glooxwrapper::string glooxwrapper::Tag::cdata() const
{
	return m_Wrapped->cdata();
}

const glooxwrapper::Tag* glooxwrapper::Tag::findTag_clone(const string& expression) const
{
	const gloox::Tag* tag = m_Wrapped->findTag(expression.to_string());
	if (!tag)
		return NULL;
	return new glooxwrapper::Tag(const_cast<gloox::Tag*>(tag), false);
}

glooxwrapper::ConstTagList glooxwrapper::Tag::findTagList_clone(const string& expression) const
{
	glooxwrapper::ConstTagList tagListWrapper;
	for (const gloox::Tag* const& t : m_Wrapped->findTagList(expression.to_string()))
		tagListWrapper.push_back(new glooxwrapper::Tag(const_cast<gloox::Tag*>(t), false));
	return tagListWrapper;
}
