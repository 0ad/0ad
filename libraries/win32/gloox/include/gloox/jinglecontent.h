/*
  Copyright (c) 2008-2017 by Jakob Schröter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/


#ifndef JINGLECONTENT_H__
#define JINGLECONTENT_H__


#include "jingleplugin.h"
#include "tag.h"

#include <string>

namespace gloox
{

  namespace Jingle
  {

    class PluginFactory;

    /**
     * @brief An abstraction of a Jingle Content Type. This is part of Jingle (@xep{0166}).
     *
     * See @link gloox::Jingle::Session Jingle::Session @endlink for more info on Jingle.
     *
     * XEP Version: 1.1
     *
     * @author Jakob Schröter <js@camaya.net>
     * @since 1.0.5
     */
    class GLOOX_API Content : public Plugin
    {
      public:
        /**
         * The original creator of the content type.
         */
        enum Creator
        {
          CInitiator,                /**< The creator is the initiator of the session. */
          CResponder,                /**< The creator is the responder. */
          InvalidCreator             /**< Invalid value. */
        };

        /**
         * The parties in the session that will be generating content.
         */
        enum Senders
        {
          SInitiator,                /**< The initiator generates/sends content. */
          SResponder,                /**< The responder generates/sends content. */
          SBoth,                     /**< Both parties generate/send content( default). */
          SNone,                     /**< No party generates/sends content. */
          InvalidSender              /**< Invalid value. */
        };

        /**
         * Creates a new Content wrapper.
         * @param name A unique name for the content type.
         * @param plugins A list of application formats, transport methods, security preconditions, ...
         * @param creator Which party originally generated the content type; the defined values are "SInitiator" and "SResponder".
         * @param senders Which parties in the session will be generating content.
         * @param disposition How the content definition is to be interpreted by the recipient. The meaning of this attribute
         * matches the "Content-Disposition" header as defined in RFC 2183 and applied to SIP by RFC 3261.
         */
        Content( const std::string& name, const PluginList& plugins, Creator creator = CInitiator,
                 Senders senders = SBoth, const std::string& disposition = "session" );

        /**
         * Creates a new Content object from the given tag.
         * @param tag The Tag to parse.
         * @param factory A PluginFactory instance to use for embedding plugins.
         */
        Content( const Tag* tag = 0, PluginFactory* factory = 0 );

        /**
         * Returns the content's creator.
         * @return The content's creator.
         */
        Creator creator() const { return m_creator; }

        /**
         * Returns the senders.
         * @return The senders.
         */
        Senders senders() const { return m_senders; }

        /**
         * Returns the disposition.
         * @return The disposition.
         */
        const std::string& disposition() const { return m_disposition; }

        /**
         * Returns the content name.
         * @return The content name.
         */
        const std::string& name() const { return m_name; }

        /**
         * Virtual destructor.
         */
        virtual ~Content();

        // reimplemented from Plugin
        virtual const std::string& filterString() const;

        // reimplemented from Plugin
        virtual Tag* tag() const;

        // reimplemented from Plugin
        virtual Plugin* newInstance( const Tag* tag ) const { return new Content( tag, m_factory ); }

        // reimplemented from Plugin
        virtual Plugin* clone() const;

      private:
        Creator m_creator;
        std::string m_disposition;
        std::string m_name;
        Senders m_senders;

    };

  }

}

#endif // JINGLECONTENT_H__
