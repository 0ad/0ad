/* Copyright (C) 2019 Wildfire Games.
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

#ifndef INCLUDED_GLOOXWRAPPER_H
#define INCLUDED_GLOOXWRAPPER_H

/*

The gloox API uses various STL types (std::string, std::list, etc), and
it has functions that acquire/release ownership of objects and expect the
library's user's 'new'/'delete' functions to be compatible with the library's.

These assumptions are invalid when the game and library are built with
different compiler versions (or the same version with different build flags):
the STL types have different layouts, and new/delete can use different heaps.

We want to let people build the game on Windows with any compiler version
(VC2008, 2010, 2012, 2013, and debug vs release), without requiring them to
rebuild the gloox library themselves. And we don't want to provide ~8 different
prebuilt versions of the library.

glooxwrapper replaces the gloox API with a version that is safe to use across
compiler versions. glooxwrapper and gloox must be compiled together with the
same version, but the resulting library can be used with any other compiler.

This is the small subset of the API that the game currently uses, with no
attempt to be comprehensive.

General design and rules:

 * There is a strict boundary between gloox+glooxwrapper.cpp, and the game
   code that includes glooxwrapper.h.
   Objects allocated with new/delete on one side of the boundary must be
   freed/allocated on the same side.
   Objects allocated with glooxwrapper_alloc()/glooxwrapper_delete() can be
   freely shared across the boundary.

 * glooxwrapper.h and users of glooxwrapper must not use any types from
   the gloox namespace, except for enums.

 * std::string is replaced with glooxwrapper::string,
   std::list with glooxwrapper::list

 * Most glooxwrapper classes are simple wrappers around gloox classes.
   Some always take ownership of their wrapped gloox object (i.e. their
   destructor will delete the wrapped object too); some never do; and some
   can be used either way (indicated by an m_Owned field).

*/

#if OS_WIN
# include "lib/sysdep/os/win/win.h"
// Prevent gloox pulling in windows.h
# define _WINDOWS_
#endif

#include <gloox/client.h>
#include <gloox/mucroom.h>
#include <gloox/registration.h>
#include <gloox/message.h>
#include <gloox/jinglecontent.h>
#include <gloox/jingleiceudp.h>
#include <gloox/jinglesessionhandler.h>
#include <gloox/jinglesessionmanager.h>

#include <cstring>

#if OS_WIN
#define GLOOXWRAPPER_API __declspec(dllexport)
#else
#define GLOOXWRAPPER_API
#endif

namespace glooxwrapper
{
	class Client;
	class DataForm;
	class DelayedDelivery;
	class Disco;
	class IQ;
	class JID;
	class MUCRoom;
	class MUCRoomConfigHandler;
	class Message;
	class MessageSession;
	class OOB;
	class Presence;
	class StanzaError;
	class StanzaExtension;
	class Tag;

	class ClientImpl;
	class MUCRoomHandlerWrapper;
	class SessionHandlerWrapper;

	GLOOXWRAPPER_API void* glooxwrapper_alloc(size_t size);
	GLOOXWRAPPER_API void glooxwrapper_free(void* p);

	class string
	{
	private:
		size_t m_Size;
		char* m_Data;
	public:
		string()
		{
			m_Size = 0;
			m_Data = (char*)glooxwrapper_alloc(1);
			m_Data[0] = '\0';
		}

		string(const string& str)
		{
			m_Size = str.m_Size;
			m_Data = (char*)glooxwrapper_alloc(m_Size + 1);
			memcpy(m_Data, str.m_Data, m_Size + 1);
		}

		string(const std::string& str) : m_Data(NULL)
		{
			m_Size = str.size();
			m_Data = (char*)glooxwrapper_alloc(m_Size + 1);
			memcpy(m_Data, str.c_str(), m_Size + 1);
		}

		string(const char* str)
		{
			m_Size = strlen(str);
			m_Data = (char*)glooxwrapper_alloc(m_Size + 1);
			memcpy(m_Data, str, m_Size + 1);
		}

		string& operator=(const string& str)
		{
			if (this != &str)
			{
				glooxwrapper_free(m_Data);
				m_Size = str.m_Size;
				m_Data = (char*)glooxwrapper_alloc(m_Size + 1);
				memcpy(m_Data, str.m_Data, m_Size + 1);
			}
			return *this;
		}

		~string()
		{
			glooxwrapper_free(m_Data);
		}

		std::string to_string() const
		{
			return std::string(m_Data, m_Size);
		}

		const char* c_str() const
		{
			return m_Data;
		}

		bool empty() const
		{
			return m_Size == 0;
		}

		bool operator==(const char* str) const
		{
			return strcmp(m_Data, str) == 0;
		}

		bool operator!=(const char* str) const
		{
			return strcmp(m_Data, str) != 0;
		}
	};

	static inline std::ostream& operator<<(std::ostream& stream, const string& string)
	{
		return stream << string.c_str();
	}

	template<typename T>
	class list
	{
	private:
		struct node
		{
			node(const T& item) : m_Item(item), m_Next(NULL) {}
			T m_Item;
			node* m_Next;
		};
		node* m_Head;
		node* m_Tail;

	public:
		struct const_iterator
		{
			const node* m_Node;
			const_iterator(const node* n) : m_Node(n) {}
			bool operator!=(const const_iterator& it) { return m_Node != it.m_Node; }
			const_iterator& operator++() { m_Node = m_Node->m_Next; return *this; }
			const T& operator*() { return m_Node->m_Item; }
		};
		const_iterator begin() const { return const_iterator(m_Head); }
		const_iterator end() const { return const_iterator(NULL); }

		list() : m_Head(NULL), m_Tail(NULL) {}

		list(const list& src) : m_Head(NULL), m_Tail(NULL)
		{
			*this = src;
		}

		list& operator=(const list& src)
		{
			if (this != &src)
			{
				clear();
				for (node* n = src.m_Head; n; n = n->m_Next)
					push_back(n->m_Item);
			}
			return *this;
		}

		~list()
		{
			clear();
		}

		void push_back(const T& item)
		{
			node* n = new (glooxwrapper_alloc(sizeof(node))) node(item);
			if (m_Tail)
				m_Tail->m_Next = n;
			m_Tail = n;
			if (!m_Head)
				m_Head = n;
		}

		void clear()
		{
			node* n = m_Head;
			while (n)
			{
				node* next = n->m_Next;
				glooxwrapper_free(n);
				n = next;
			}
			m_Head = m_Tail = NULL;
		}

		const T& front() const
		{
			return *begin();
		}
	};

	typedef glooxwrapper::list<Tag*> TagList;
	typedef glooxwrapper::list<const Tag*> ConstTagList;

	struct CertInfo
	{
		int status;
		bool chain;
		string issuer;
		string server;
		int date_from;
		int date_to;
		string protocol;
		string cipher;
		string mac;
		string compression;
	};

	struct RegistrationFields
	{
		string username;
		string nick;
		string password;
		string name;
		string first;
		string last;
		string email;
		string address;
		string city;
		string state;
		string zip;
		string phone;
		string url;
		string date;
		string misc;
		string text;
	};

	struct MUCRoomParticipant
	{
		JID* nick;
		gloox::MUCRoomAffiliation affiliation;
		gloox::MUCRoomRole role;
		JID* jid;
		int flags;
		string reason;
		JID* actor;
		string newNick;
		string status;
		JID* alternate;
	};


	class GLOOXWRAPPER_API ConnectionListener
	{
	public:
		virtual ~ConnectionListener() {}
		virtual void onConnect() = 0;
		virtual void onDisconnect(gloox::ConnectionError e) = 0;
		virtual bool onTLSConnect(const CertInfo& info) = 0;
	};

	class GLOOXWRAPPER_API IqHandler
	{
	public:
		virtual ~IqHandler() {}
		virtual bool handleIq(const IQ& iq) = 0;
		virtual void handleIqID(const IQ& iq, int context) = 0;
	};

	class GLOOXWRAPPER_API MessageHandler
	{
	public:
		virtual ~MessageHandler() {}
		virtual void handleMessage(const Message& msg, MessageSession* session = 0) = 0; // MessageSession not supported
	};

	class GLOOXWRAPPER_API MUCRoomHandler
	{
	public:
		virtual ~MUCRoomHandler() {}
		virtual void handleMUCParticipantPresence(MUCRoom* room, const MUCRoomParticipant participant, const Presence& presence) = 0; // MUCRoom not supported
		virtual void handleMUCMessage(MUCRoom* room, const Message& msg, bool priv) = 0; // MUCRoom not supported
		virtual void handleMUCError(MUCRoom* room, gloox::StanzaError error) = 0; // MUCRoom not supported
		virtual void handleMUCSubject(MUCRoom* room, const string& nick, const string& subject) = 0; // MUCRoom not supported
	};

	class GLOOXWRAPPER_API RegistrationHandler
	{
	public:
		virtual ~RegistrationHandler() {}
		virtual void handleRegistrationFields(const JID& from, int fields, string instructions) = 0;
		virtual void handleAlreadyRegistered(const JID& from) = 0;
		virtual void handleRegistrationResult(const JID& from, gloox::RegistrationResult regResult) = 0;
		virtual void handleDataForm(const JID& from, const DataForm& form) = 0; // DataForm not supported
		virtual void handleOOB(const JID& from, const OOB& oob) = 0; // OOB not supported
	};

	class GLOOXWRAPPER_API StanzaExtension
	{
	public:
		StanzaExtension(int type) : m_extensionType(type) {}
		virtual ~StanzaExtension() {}
		virtual const string& filterString() const = 0;
		virtual StanzaExtension* newInstance(const Tag* tag) const = 0;
		virtual glooxwrapper::Tag* tag() const = 0;
		virtual StanzaExtension* clone() const = 0;

		int extensionType() const { return m_extensionType; }
	private:
		int m_extensionType;
	};


	class GLOOXWRAPPER_API Client
	{
		NONCOPYABLE(Client);
	private:
		gloox::Client* m_Wrapped;
		ClientImpl* m_Impl;
		Disco* m_DiscoWrapper;

	public:
		gloox::Client* getWrapped() { return m_Wrapped; }

		bool connect(bool block = true);
		gloox::ConnectionError recv(int timeout = -1);
		const string getID() const;
		void send(const IQ& iq);

		void setTls(gloox::TLSPolicy tls);
		void setCompression(bool compression);

		void setSASLMechanisms(int mechanisms);
		void registerStanzaExtension(StanzaExtension* ext);
		void registerConnectionListener(ConnectionListener* cl);
		void registerIqHandler(IqHandler* ih, int exttype);
		void registerMessageHandler(MessageHandler* mh);

		bool removePresenceExtension(int type);

		Disco* disco() const { return m_DiscoWrapper; }

		Client(const string& server);
		Client(const JID& jid, const string& password, int port = -1);
		~Client();

		void setPresence(gloox::Presence::PresenceType pres, int priority, const string& status = "");
		void disconnect();
	};

	class GLOOXWRAPPER_API DelayedDelivery
	{
		NONCOPYABLE(DelayedDelivery);
	private:
		const gloox::DelayedDelivery* m_Wrapped;
	public:
		DelayedDelivery(const gloox::DelayedDelivery* wrapped);
		const string stamp() const;
	};

	class GLOOXWRAPPER_API Disco
	{
		NONCOPYABLE(Disco);
	private:
		gloox::Disco* m_Wrapped;
	public:
		Disco(gloox::Disco* wrapped);
		void setVersion(const string& name, const string& version, const string& os = "");
		void setIdentity(const string& category, const string& type, const string& name = "");
	};

	class GLOOXWRAPPER_API IQ
	{
		NONCOPYABLE(IQ);
	private:
		gloox::IQ* m_Wrapped;
		bool m_Owned;
	public:
		const gloox::IQ& getWrapped() const { return *m_Wrapped; }
		IQ(const gloox::IQ& iq) : m_Wrapped(const_cast<gloox::IQ*>(&iq)), m_Owned(false) { }

		IQ(gloox::IQ::IqType type, const JID& to, const string& id);
		~IQ();

		void addExtension(const StanzaExtension* se);
		const StanzaExtension* findExtension(int type) const;

		template<class T>
		inline const T* findExtension(int type) const
		{
			return static_cast<const T*>(findExtension(type));
		}

		gloox::IQ::IqType subtype() const;
		const string id() const;
		const gloox::JID& from() const;

		gloox::StanzaError error_error() const; // wrapper for ->error()->error()
		Tag* tag() const;
	};

	class GLOOXWRAPPER_API JID
	{
		NONCOPYABLE(JID);
	private:
		gloox::JID* m_Wrapped;
		bool m_Owned;
		void init(const char* data, size_t len);
	public:
		const gloox::JID& getWrapped() const { return *m_Wrapped; }
		JID(const gloox::JID& jid) : m_Wrapped(const_cast<gloox::JID*>(&jid)), m_Owned(false) { }

		JID();
		JID(const string& jid);
		JID(const std::string& jid) { init(jid.c_str(), jid.size()); }
		~JID();

		string username() const;
		string resource() const;
	};

	class GLOOXWRAPPER_API Message
	{
		NONCOPYABLE(Message);
	private:
		gloox::Message* m_Wrapped;
		bool m_Owned;
		glooxwrapper::JID* m_From;
	public:
		Message(gloox::Message* wrapped, bool owned);
		~Message();
		gloox::Message::MessageType subtype() const;
		const JID& from() const;
		string body() const;
		string subject(const string& lang = "default") const;
		string thread() const;
		const glooxwrapper::DelayedDelivery* when() const;
	};

	class GLOOXWRAPPER_API MUCRoom
	{
		NONCOPYABLE(MUCRoom);
	private:
		gloox::MUCRoom* m_Wrapped;
		MUCRoomHandlerWrapper* m_HandlerWrapper;
	public:
		MUCRoom(Client* parent, const JID& nick, MUCRoomHandler* mrh, MUCRoomConfigHandler* mrch = 0);
		~MUCRoom();
		const string nick() const;
		void join(gloox::Presence::PresenceType type = gloox::Presence::Available, const string& status = "", int priority = 0);
		void leave(const string& msg = "");
		void send(const string& message);
		void setNick(const string& nick);
		void setPresence(gloox::Presence::PresenceType presence, const string& msg = "");
		void setRequestHistory(int value, gloox::MUCRoom::HistoryRequestType type);
		void kick(const string& nick, const string& reason);
		void ban(const string& nick, const string& reason);
	};

	class GLOOXWRAPPER_API Presence
	{
		gloox::Presence::PresenceType m_Presence;
	public:
		Presence(gloox::Presence::PresenceType presence) : m_Presence(presence) {}
		gloox::Presence::PresenceType presence() const { return m_Presence; }
	};

	class GLOOXWRAPPER_API Registration
	{
		NONCOPYABLE(Registration);
	private:
		gloox::Registration* m_Wrapped;
		std::list<shared_ptr<gloox::RegistrationHandler> > m_RegistrationHandlers;
	public:
		Registration(Client* parent);
		~Registration();
		void fetchRegistrationFields();
		bool createAccount(int fields, const RegistrationFields& values);
		void registerRegistrationHandler(RegistrationHandler* rh);
	};

	class GLOOXWRAPPER_API Tag
	{
		NONCOPYABLE(Tag);
	private:
		gloox::Tag* m_Wrapped;
		bool m_Owned;

		Tag(const string& name);
		Tag(const string& name, const string& cdata);
		Tag(gloox::Tag* wrapped, bool owned) : m_Wrapped(wrapped), m_Owned(owned) {}
		~Tag();

	public:
		// Internal use:
		gloox::Tag* getWrapped() { return m_Wrapped; }
		gloox::Tag* stealWrapped() { m_Owned = false; return m_Wrapped; }
		static Tag* allocate(gloox::Tag* wrapped, bool owned);

		// Instead of using new/delete, Tags must be allocated/freed with these functions
		static Tag* allocate(const string& name);
		static Tag* allocate(const string& name, const string& cdata);
		static void free(const Tag* tag);

		bool addAttribute(const string& name, const string& value);
		string findAttribute(const string& name) const;
		Tag* clone() const;
		string xmlns() const;
		bool setXmlns(const string& xmlns);
		string xml() const;
		void addChild(Tag* child);
		string name() const;
		string cdata() const;
		const Tag* findTag_clone(const string& expression) const; // like findTag but must be Tag::free()d
		ConstTagList findTagList_clone(const string& expression) const; // like findTagList but each tag must be Tag::free()d
	};

	/**
	 * See XEP-0166: Jingle and https://camaya.net/api/gloox/namespacegloox_1_1Jingle.html.
	 */
	namespace Jingle
	{

		class GLOOXWRAPPER_API Plugin
		{
		protected:
			const gloox::Jingle::Plugin* m_Wrapped;
			bool m_Owned;

		public:
			Plugin(const gloox::Jingle::Plugin* wrapped, bool owned) : m_Wrapped(wrapped), m_Owned(owned) {}

			virtual ~Plugin();

			const Plugin findPlugin(int type) const;
			const gloox::Jingle::Plugin* getWrapped() const { return m_Wrapped; }
		};

		typedef list<const Plugin*> PluginList;

		/**
		 * See XEP-0176: Jingle ICE-UDP Transport Method
		 */
		class GLOOXWRAPPER_API ICEUDP
		{
		public:
			struct Candidate {
				string ip;
				int port;
			};
		private:
			// Class not implemented as it is not used.
			ICEUDP() = delete;
		};

		class GLOOXWRAPPER_API Session
		{
		protected:
			gloox::Jingle::Session* m_Wrapped;
			bool m_Owned;

		public:
			class GLOOXWRAPPER_API Jingle
			{
			private:
				const gloox::Jingle::Session::Jingle* m_Wrapped;
				bool m_Owned;
			public:
				Jingle(const gloox::Jingle::Session::Jingle* wrapped, bool owned) : m_Wrapped(wrapped), m_Owned(owned) {}
				~Jingle()
				{
					if (m_Owned)
						delete m_Wrapped;
				}
				ICEUDP::Candidate getCandidate() const;
			};

			Session(gloox::Jingle::Session* wrapped, bool owned);
			~Session();

			bool sessionInitiate(char* ipStr, uint16_t port);
		};

		class GLOOXWRAPPER_API SessionHandler
		{
		public:
			virtual ~SessionHandler() {}
			virtual void handleSessionAction(gloox::Jingle::Action action, Session& session, const Session::Jingle& jingle) = 0;
		};

	}

	class GLOOXWRAPPER_API SessionManager
	{
	private:
		gloox::Jingle::SessionManager* m_Wrapped;
		SessionHandlerWrapper* m_HandlerWrapper;

	public:
		SessionManager(Client* parent, Jingle::SessionHandler* sh);
		~SessionManager();
		void registerPlugins();
		Jingle::Session createSession(const JID& callee);
	};

}

#endif // INCLUDED_GLOOXWRAPPER_H
