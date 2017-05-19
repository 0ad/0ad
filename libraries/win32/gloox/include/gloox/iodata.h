/*
  Copyright (c) 2015-2017 by Jakob Schröter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/


#ifndef IODATA_H__
#define IODATA_H__

#include "adhocplugin.h"

#include "gloox.h"
#include "tag.h"

#include <string>

namespace gloox
{

  /**
   * @brief This is an abstraction of the IO Data specification @xep{0244}.
   *
   * This abstraction can be used to implement IO Data on top of Data Forms.
   *
   * XEP version: 0.1
   * @author Jakob Schröter <js@camaya.net>
   * @since 1.0.13
   */
  class GLOOX_API IOData : public AdhocPlugin
  {
    public:
      /**
       * The IO Data transaction types.
       */
      enum Type
      {
        TypeIoSchemataGet,           /** To request the schemata of input and output. */
        TypeInput,                   /** To submit the input. */
        TypeGetStatus,               /** To request the status of the procedure. */
        TypeGetOutput,               /** To request the output. */
        TypeIoSchemataResult,        /** To return the schemata of input and output. */
        TypeOutput,                  /** To submit the output. */
        TypeError,                   /** To submit additional error information. */
        TypeStatus,                  /** To indicate the current status of the procedure. */
        TypeInvalid                  /** Invalid type. */
      };

      struct Status
      {
        int elapsed;                 /** Aan integer value of the time in milliseconds that
                                      * elapsed since the procedure was invoked. */
        int remaining;               /** An integer value of the (estimated) time in milliseconds
                                      * till the procedure will finish. */
        int percentage;              /** The percentage of the procedure that is finished. */
        std::string info;            /** Describes the current status of the procedure. */
      };

      /**
       * Constructs a new IO Data object of the given type.
       * @param type The transaction type.
       */
      IOData( Type type );

      /**
       * Constructs a new IO Data object by parsing the given Tag.
       * @param tag The Tag to parse. This should be a &lt;iodata&gt; tag with the correct namespace and child elements.
       */
      IOData( const Tag* tag );

      /**
       * Virtual destructor.
       */
      virtual ~IOData();

      /**
       * Returns the IO Data object's type.
       * @return The IO Data object's type.
       */
      Type type() const { return m_type; }

      /**
       * Returns the 'input' tag, if the transaction type is either @c input or @c io-schemata-result.
       * @return The 'input' tag, including the encapsulating &lt;in&gt;.
       * @note The IOData instance will still own the tag and delete it. Clone it if you need it later.
       */
      const Tag* in() const { return m_in; }

      /**
       * Sets the 'input' tag. If an 'input' tag was previosuly set, it is deleted before the new one is set.
       * Alternatively, if your input consists of more than one element, you can embed these into an
       * &lt;in&gt; tag with no namespace.
       * @param in The new 'input' tag.
       * @note The @c in tag will be owned by this IOData instance. Clone it if you need it somewhere else.
       */
      void setIn( Tag* in );

      /**
       * Returns the 'output' tag, if the transaction type is either @c output or @c io-schemata-result.
       * @return The 'output' tag, including the encapsulating &lt;out&gt;.
       * @note The IOData instance will still own the tag and delete it. Clone it if you need it later.
       */
      const Tag* out() const { return m_out; }

      /**
       * Sets the 'output' tag. If an 'output' tag was previosuly set, it is deleted before the new one is set.
       * Alternatively, if your output consists of more than one element, you can embed these into an
       * &lt;out&gt; tag with no namespace.
       * @param out The new 'output' tag.
       * @note The @c out tag will be owned by this IOData instance. Clone it if you need it somewhere else.
       */
      void setOut( Tag* out );

      /**
       * Returns the 'error' tag, if the transaction type is either @c error or @c io-schemata-result.
       * @return The 'error' tag, including the encapsulating &lt;error&gt;.
       * @note The IOData instance will still own the tag and delete it. Clone it if you need it later.
       */
      const Tag* error() const { return m_error; }

      /**
       * Sets the 'error' tag. If an 'error' tag was previosuly set, it is deleted before the new one is set.
       * Alternatively, if your error consists of more than one element, you can embed these into an
       * &lt;error&gt; tag with no namespace.
       * @param error The new 'error' tag.
       * @note The @c error tag will be owned by this IOData instance. Clone it if you need it somewhere else.
       */
      void setError( Tag* error );

      /**
       * Sets the Schema description. Only used/valid if type is @c io-schemata-result.
       * @param desc The schema description.
       */
      void setDesc( const std::string& desc ) { m_desc = desc; }

      /**
       * Returns the schema description, if any. Usually only valid if transaction type is @c io-schema-result.
       * @return The schema description.
       */
      const std::string& desc() const { return m_desc; }

      /**
       * Sets the status of the procedure. Only used/valid if transaction type is @c status.
       * @param status The status of the procedure.
       */
      void setStatus( Status status ) { m_status = status; }

      /**
       * Returns the status of the procedure. Only used/valid if transaction type is @c status.
       * @return The status of the procedure.
       */
      Status status() const { return m_status; }

      // reimplemented from AdhocPlugin/StanzaExtension
      virtual Tag* tag() const;

      // reimplemented from AdhocPlugin/StanzaExtension
      virtual IOData* clone() const;

      // reimplemented from AdhocPlugin/StanzaExtension
      virtual const std::string& filterString() const { return EmptyString; }

      // reimplemented from AdhocPlugin/StanzaExtension
      virtual StanzaExtension* newInstance( const Tag* /*tag*/ ) const { return 0; }

      /**
       * Converts to  @b true if the IOData is valid, @b false otherwise.
       */
      operator bool() const { return m_type != TypeInvalid; }

  private:
    Tag* m_in;
    Tag* m_out;
    Tag* m_error;

    std::string m_desc;

    Status m_status;

    Type m_type;

  };

}

#endif // IODATA_H__
