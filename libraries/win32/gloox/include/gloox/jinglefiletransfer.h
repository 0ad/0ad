/*
  Copyright (c) 2013-2017 by Jakob Schröter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox

  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.

  This software is distributed without any warranty.
*/


#ifndef JINGLEFILETRANSFER_H__
#define JINGLEFILETRANSFER_H__

#include "jingleplugin.h"
#include "tag.h"

#include <string>
#include <list>

namespace gloox
{

  namespace Jingle
  {

    /**
     * @brief An abstraction of the signaling part of Jingle File Transfer (@xep{0234}), implemented as a Jingle::Plugin.
     *
     * XEP Version: 0.15
     *
     * @author Jakob Schröter <js@camaya.net>
     * @since 1.0.7
     */
    class GLOOX_API FileTransfer : public Plugin
    {
      public:

        /**
         * The type of a FileTransfer instance.
         */
        enum Type
        {
          Offer,                    /**< Signifies a file transfer offer (send). */
          Request,                  /**< Signifies a file request (pull). */
          Checksum,                 /**< Used to send a file's checksum. */
          Abort,                    /**< used to abort a running transfer. */
          Received,                 /**< Signifies a successful file transfer. */
          Invalid                   /**< Invalid type. */
        };

        /**
         * A struct holding information about a file.
         */
        struct File
        {
          std::string name;         /**< The file's name. */
          std::string date;         /**< The file's (creation?) date */
          std::string desc;         /**< A description. */
          std::string hash;         /**< The file's cehcksum. */
          std::string hash_algo;    /**< The algorithm used to calculate the checksum */
          long int size;            /**< The filesize in Bytes. */
          bool range;               /**< Signifies that an offset transfer is possible. */
          long int offset;          /**< An (optional) offset. */
        };

        /** A list of file information structs. */
        typedef std::list<File> FileList;

        /**
         * Creates a new instance.
         * @param type The type of the object.
         * @param files A list of files to offer, request, acknowledge, ... Most of
         * the time this list will contain only one file.
         */
        FileTransfer( Type type, const FileList& files );

        /**
         * Creates a new instance from the given Tag
         * @param tag The Tag to parse.
         */
        FileTransfer( const Tag* tag = 0 );

        /**
         * Virtual destructor.
         */
        virtual ~FileTransfer() {}

        /**
         * Returns the type.
         * @return The type.
         */
        Type type() const { return m_type; }

        /**
         * Returns a list of embedded file infos.
         * @return A list of embedded file infos.
         */
        const FileList& files() const { return m_files; }

        // reimplemented from Plugin
        virtual const StringList features() const;

        // reimplemented from Plugin
        virtual const std::string& filterString() const;

        // reimplemented from Plugin
        virtual Tag* tag() const;

        // reimplemented from Plugin
        virtual Plugin* newInstance( const Tag* tag ) const;

        // reimplemented from Plugin
        virtual Plugin* clone() const
        {
          return new FileTransfer( *this );
        }

      private:

        void parseFileList( const TagList& files );

        Type m_type;
        FileList m_files;

    };

  }

}

#endif // JINGLEFILETRANSFER_H__
