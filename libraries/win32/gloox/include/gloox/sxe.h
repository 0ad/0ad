/*
  Copyright (c) 2019 by Jakob Schröter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/

#ifndef SXE_H__
#define SXE_H__

#include "stanzaextension.h"

#include <memory>
#include <string>
#include <vector>

namespace gloox
{

  enum SxeType
  {
    SxeInvalid,
    SxeConnect,
    SxeStateOffer,
    SxeAcceptState,
    SxeRefuseState,
    SxeState
  };

  enum StateChangeType
  {
    StateChangeDocumentBegin,
    StateChangeDocumentEnd,
    StateChangeNew,
    StateChangeRemove,
    StateChangeSet
  };

  struct DocumentBegin
  {
    const char* prolog;
  };

  struct DocumentEnd
  {
    const char* last_sender;
    const char* last_id;
  };

  struct New
  {
    const char* rid;
    const char* type;
    const char* name;
    const char* ns;
    const char* parent;
    const char* chdata;
  };

  struct Remove
  {
    const char* target;
  };

  struct Set
  {
    const char* target;
    const char* version;
    const char* parent;
    const char* name;
    const char* ns;
    const char* chdata;
  };

  struct StateChange
  {
    StateChangeType type;
    union
    {
      DocumentBegin document_begin;
      DocumentEnd document_end;
      New new_;
      Remove remove;
      Set set;
    };
  };

  /**
   * @brief An implementation/abstraction of Shared XML Editing (SXE, @xep{0284})
   *
   * XEP Version: 0.1.1
   *
   * @author Emmanuel Gil Peyrot <linkmauve@linkmauve.fr>
   * @since 1.0.23
   */
  class GLOOX_API Sxe : public StanzaExtension
  {
    private:
      Sxe( std::string session, std::string id, SxeType type, std::vector<std::string> state_offer_xmlns, std::vector<StateChange> state_changes );

    public:
      /**
       * Creates a new SXE object from the given Tag.
       * @param tag The Tag to parse.
       */
      Sxe( const Tag* tag = 0 );

      /**
       * Virtual destructor.
       */
      virtual ~Sxe() {}

      /**
       * Returns a Tag representing a SXE extension.
       * @return A Tag representing a SXE extension.
       */
      virtual Tag* tag() const;

      /**
       * Returns a new instance of SXE.
       * @return The new SXE instance.
       */
      virtual StanzaExtension* newInstance( const Tag* tag ) const
      {
        return new Sxe( tag );
      }

      /**
       * Returns an identical copy of the current SXE.
       * @return an identical copy of the current SXE.
       */
      virtual StanzaExtension* clone() const
      {
        return new Sxe( *this );
      }

      /**
       * Returns an XPath expression that describes a path to the SXE element.
       * @return The SXE filter string.
       */
      virtual const std::string& filterString() const;

    private:
      std::string m_session;
      std::string m_id;
      SxeType m_type;
      std::vector<std::string> m_state_offer_xmlns;
      std::vector<StateChange> m_state_changes;

  };

}

#endif // SXE_H__
