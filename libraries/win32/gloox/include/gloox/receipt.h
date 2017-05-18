/*
  Copyright (c) 2007-2017 by Jakob Schröter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/

#ifndef RECEIPT_H__
#define RECEIPT_H__

#include "gloox.h"
#include "stanzaextension.h"

#include <string>

namespace gloox
{

  class Tag;

  /**
   * @brief An implementation of Message Receipts (@xep{0184}) as a StanzaExtension.
   *
   * @author Jakob Schröter <js@camaya.net>
   * @since 1.0
   */
  class GLOOX_API Receipt : public StanzaExtension
  {
    public:
      /**
       * Contains valid receipt types (@xep{0184}).
       */
      enum ReceiptType
      {
        Request,                    /**< Requests a receipt. */
        Received,                   /**< The receipt. */
        Invalid                     /**< Invalid type. */
      };

      /**
       * Constructs a new object from the given Tag.
       * @param tag A Tag to parse.
       */
      Receipt( const Tag* tag );

      /**
       * Constructs a new object of the given type.
       * @param rcpt The receipt type.
       * @param id The message ID.
       */
      Receipt( ReceiptType rcpt, const std::string& id = EmptyString )
        : StanzaExtension( ExtReceipt ), m_rcpt( rcpt ), m_id( id )
      {}

      /**
       * Virtual destructor.
       */
      virtual ~Receipt() {}

      /**
       * Returns the object's state.
       * @return The object's state.
       */
      ReceiptType rcpt() const { return m_rcpt; }

      /**
       * Returns the message id for acknowledgement tracking.
       * @return The message ID for acknowledgement tracking.
       */
      std::string id() const { return m_id; }

      // reimplemented from StanzaExtension
      virtual const std::string& filterString() const;

      // reimplemented from StanzaExtension
      virtual StanzaExtension* newInstance( const Tag* tag ) const
      {
        return new Receipt( tag );
      }

      // reimplemented from StanzaExtension
      Tag* tag() const;

      // reimplemented from StanzaExtension
      virtual StanzaExtension* clone() const
      {
        return new Receipt( *this );
      }

    private:
      ReceiptType m_rcpt;
      std::string m_id;

  };

}

#endif // RECEIPT_H__
