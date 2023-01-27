/*
 *  Copyright (c) 2018-2019 by Jakob Schröter <js@camaya.net>
 *  This file is part of the gloox library. http://camaya.net/gloox
 *
 *  This software is distributed under a license. The full license
 *  agreement can be found in the file LICENSE in this distribution.
 *  This software may not be copied, modified, sold or distributed
 *  other than expressed in the named license agreement.
 *
 *  This software is distributed without any warranty.
 */


#if !defined( GLOOX_MINIMAL ) || defined( WANT_MESSAGEMARKUP )

#ifndef MESSAGEMARKUP_H__
#define MESSAGEMARKUP_H__

#include "gloox.h"
#include "stanzaextension.h"

#include <string>
#include <list>

namespace gloox
{
  /**
   * This is an abstraction of Message Markup (@xep{0394}).
   *
   * XEP Version: 0.1.0
   *
   * @author Jakob Schröter <js@camaya.net>
   * @since 1.1
   */
  class GLOOX_API MessageMarkup : public StanzaExtension
  {
    public:

      /**
       * A list of ints.
       */
      typedef std::list<int> IntList;

      /**
       * The supported Message Markup types according to @xep{0394}.
       */
      enum MarkupType
      {
        Span,                       /**< A span, possibly containing emphasis, code and/or deleted tags. */
        BCode,                      /**< A code block. Suggested rendering: as block-level element with
                                     * monospaced font. */
        List,                       /**< An itemized list. */
        BQuote,                     /**< A block quote. */
        InvalidType                 /**< Invalid/unknown type. Ignored. */
      };

      /**
       * The supported Span types according to @xep{0394}.
       */
      enum SpanType
      {
        Emphasis    =  1,           /**< An emphasised section of text. */
        Code        =  2,           /**< Suggested rendering: with monospaced font. */
        Deleted     =  4,           /**< Deleted text. Suggested rendering strike-through. */
        InvalidSpan =  8            /**< Invalid/unknown type. Ignored. */
      };

      /**
       * A struct describing a single child element of a &lt;markup&gt; element.
       */
      struct Markup
      {
        MarkupType type;            /**< The type. */
        int start;                  /**< The start of the range, in units of unicode code points in the character
                                     * data of the body element. */
        int end;                    /**< The end of the range, in units of unicode code points in the character
                                     * data of the body element. */
        int spans;                  /**< The applicable span types (SpanType) for the current span. Only set if
                                     * @c type is @c Span, 0 otherwise. */
        IntList lis;             /**< A list of positions inside the text that are start positions for list
                                     * items. Only used if @c type is List, empty otherwise. */

        /**
         * Constructor for a Markup struct for convenience.
         * @param _type The type.
         * @param _start The start of the range.
         * @param _end The end of the range.
         * @param _spans The applicable span types (SpanType) for the current span.
         * @param _lis A list of positions inside the text that are start positions for list
         * items.
         */
        Markup( MarkupType _type, int _start, int _end, int _spans, IntList _lis )
          : type( _type ), start( _start ), end( _end ), spans( _spans ), lis( _lis )
        {}

      };

      /**
       * A list of Markup structs.
       */
      typedef std::list<Markup> MarkupList;

      /**
       * Constructs a new object from the given Tag.
       * @param tag A Tag to parse.
       */
      MessageMarkup( const Tag* tag );

      /**
       * Constructs a new MessageMarkup object with the given markups.
       * @param ml The markups to include.
       */
      MessageMarkup( const MarkupList& ml )
        : StanzaExtension( ExtMessageMarkup ), m_markups( ml )
      {}

      /**
       * Virtual destructor.
       */
      virtual ~MessageMarkup() {}

      /**
       * Lets you set the @c lang attribute (@c xml:lang) to that of the corresponding body element.
       * @param lang The language.
       */
      void setLang( const std::string& lang ) { m_lang = lang; }

      /**
       * Lets you retrieve the @c lang attribute's value (@c xml:lang).
       * @return The @c lang attribute's value.
       */
      const std::string& lang() const { return m_lang; }

      /**
       * Returns the list of markups.
       * @return The list of markups.
       */
      const MarkupList& markup() const { return m_markups; }

      // reimplemented from StanzaExtension
      virtual const std::string& filterString() const;

      // reimplemented from StanzaExtension
      virtual StanzaExtension* newInstance( const Tag* tag ) const
      {
        return new MessageMarkup( tag );
      }

      // reimplemented from StanzaExtension
      Tag* tag() const;

      // reimplemented from StanzaExtension
      virtual MessageMarkup* clone() const
      {
        return new MessageMarkup( *this );
      }

    private:
      MarkupList m_markups;
      std::string m_lang;

  };

}

#endif // MESSAGEMARKUP_H__

#endif // GLOOX_MINIMAL
