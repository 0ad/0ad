/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2001-2003 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Xerces" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache\@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation, and was
 * originally based on software copyright (c) 1999, International
 * Business Machines, Inc., http://www.ibm.com .  For more information
 * on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

/*
 * $Id: XMLUri.hpp,v 1.16 2004/01/12 22:01:02 cargilld Exp $
 * $Log: XMLUri.hpp,v $
 * Revision 1.16  2004/01/12 22:01:02  cargilld
 * Minor performance change for handling reserved and unreserved characters.
 *
 * Revision 1.15  2003/12/17 00:18:35  cargilld
 * Update to memory management so that the static memory manager (one used to call Initialize) is only for static data.
 *
 * Revision 1.14  2003/12/11 22:21:25  neilg
 * fixes for the URI implementation to take registry names into account; much thanks to Michael Glavassevich
 *
 * Revision 1.13  2003/12/02 17:50:21  neilg
 * additional fix for bug 25118; once again, thanks to Jeroen Whitmond
 *
 * Revision 1.12  2003/10/01 00:20:41  knoaman
 * Add a static method to check whether a given string is a valid URI.
 *
 * Revision 1.11  2003/09/25 22:23:25  peiyongz
 * Implementation of Serialization/Deserialization
 *
 * Revision 1.10  2003/07/25 10:15:16  gareth
 * Patch by Michael Glavassevich
 *
 * The patch fixes Bugzilla #19787, #20006, #20009, #20010 and #20287, and
 * several other issues. A summary of the changes is listed below:
 *
 * 1. Added '[' and ']' to reserved characters as per RFC 2732.
 * 2. '[' and ']' added in RFC 2732, are not allowed in path segments, but
 * may appear in the opaque part.
 * 3. No URI can begin with a ':'.
 * 4. URI has no scheme if ':' occurs in a URI after '?' or '#', it's part of
 * the query string or fragment.
 * 5. Whitespace (even escaped as %20) is not permitted in the authority
 * portion of a URI.
 * 6. IPv4 addresses must match 1*3DIGIT "." 1*3DIGIT "." 1*3DIGIT "."
 * 1*3DIGIT. Since RFC 2732.
 * 7. IPv4 addresses are 32-bit, therefore no segment may be larger than 255.
 * This isn't expressed by the grammar.
 * 8. Hostnames cannot end with a '-'.
 * 9. Labels in a hostname must be 63 bytes or less [RFC 1034].
 * 10. Hostnames may be no longer than 255 bytes [RFC 1034]. (That
 * restriction was already there. I just moved it inwards.
 * 11. Added support for IPv6 references added in RFC 2732. URIs such as
 * http://[::ffff:1.2.3.4] are valid. The BNF in RFC 2373 isn't correct. IPv6
 * addresses are read according to section 2.2 of RFC 2373.
 *
 * Revision 1.9  2003/05/16 06:01:53  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.8  2003/05/15 19:07:46  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.7  2003/01/06 19:43:18  tng
 * New feature StandardUriConformant to force strict standard uri conformance.
 *
 * Revision 1.6  2002/11/21 15:42:39  gareth
 * Implemented copy constructor and operator =. Patch by Jennifer Schachter.
 *
 * Revision 1.5  2002/11/04 15:22:05  tng
 * C++ Namespace Support.
 *
 * Revision 1.4  2002/09/23 18:41:00  tng
 * DOM L3: Support baseURI.   Add fURIText to XMLUri.   Added by Gareth Reakes and Thomas Ford.
 *
 * Revision 1.3  2002/08/23 20:45:24  tng
 * .Memory leak fix: XMLUri data not deleted if constructor failed.
 *
 * Revision 1.2  2002/02/20 18:17:02  tng
 * [Bug 5977] Warnings on generating apiDocs.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:17  peiyongz
 * sane_include
 *
 * Revision 1.3  2001/08/29 19:03:03  peiyongz
 * Bugzilla# 2816:on AIX 4.2, xlC 3 r ev.1, Compilation error on inline method
 *
 * Revision 1.2  2001/08/16 14:09:44  peiyongz
 * Removed unused ctors and methods
 *
 * Revision 1.1  2001/08/10 16:23:41  peiyongz
 * XMLUri: creation
 *
 *
 */

#if !defined(XMLURI_HPP)
#define XMLURI_HPP

#include <xercesc/util/XMemory.hpp>
#include <xercesc/util/XMLString.hpp>

#include <xercesc/internal/XSerializable.hpp>

XERCES_CPP_NAMESPACE_BEGIN

/*
 * This class is a direct port of Java's URI class, to distinguish
 * itself from the XMLURL, we use the name XMLUri instead of
 * XMLURI.
 *
 * TODO: how to relate XMLUri and XMLURL since URL is part of URI.
 *
 */

class XMLUTIL_EXPORT XMLUri : public XSerializable, public XMemory
{
public:

    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------

    /**
     * Construct a new URI from a URI specification string.
     *
     * If the specification follows the "generic URI" syntax, (two slashes
     * following the first colon), the specification will be parsed
     * accordingly - setting the
     *                           scheme,
     *                           userinfo,
     *                           host,
     *                           port,
     *                           path,
     *                           querystring and
     *                           fragment
     * fields as necessary.
     *
     * If the specification does not follow the "generic URI" syntax,
     * the specification is parsed into a
     *                           scheme and
     *                           scheme-specific part (stored as the path) only.
     *
     * @param uriSpec the URI specification string (cannot be null or empty)
     *
     * @param manager Pointer to the memory manager to be used to
     *                allocate objects.
     *
     * ctor# 2
     *
     */
    XMLUri(const XMLCh* const    uriSpec,
           MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

    /**
     * Construct a new URI from a base URI and a URI specification string.
     * The URI specification string may be a relative URI.
     *
     * @param baseURI the base URI (cannot be null if uriSpec is null or
     *                empty)
     *
     * @param uriSpec the URI specification string (cannot be null or
     *                empty if base is null)
     *
     * @param manager Pointer to the memory manager to be used to
     *                allocate objects.
     *
     * ctor# 7 relative ctor
     *
     */
    XMLUri(const XMLUri* const  baseURI
         , const XMLCh* const   uriSpec
         , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

    /**
     * Copy constructor
     */
    XMLUri(const XMLUri& toCopy);
    XMLUri& operator=(const XMLUri& toAssign);

    virtual ~XMLUri();

    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    /**
     * Get the URI as a string specification. See RFC 2396 Section 5.2.
     *
     * @return the URI string specification
     */
    const XMLCh* getUriText() const;

    /**
     * Get the scheme for this URI.
     *
     * @return the scheme for this URI
     */
     const XMLCh* getScheme() const;

    /**
     * Get the userinfo for this URI.
     *
     * @return the userinfo for this URI (null if not specified).
     */
     const XMLCh* getUserInfo() const;


    /**
     * Get the host for this URI.
     *
     * @return the host for this URI (null if not specified).
     */
     const XMLCh* getHost() const;

    /**
     * Get the port for this URI.
     *
     * @return the port for this URI (-1 if not specified).
     */
     int getPort() const;
     
    /**
     * Get the registry based authority for this URI.
     * 
     * @return the registry based authority (null if not specified).
     */
     const XMLCh* getRegBasedAuthority() const;

    /**
     * Get the path for this URI. Note that the value returned is the path
     * only and does not include the query string or fragment.
     *
     * @return the path for this URI.
     */
     const XMLCh* getPath() const;

    /**
     * Get the query string for this URI.
     *
     * @return the query string for this URI. Null is returned if there
     *         was no "?" in the URI spec, empty string if there was a
     *         "?" but no query string following it.
     */
     const XMLCh* getQueryString() const;

    /**
     * Get the fragment for this URI.
     *
     * @return the fragment for this URI. Null is returned if there
     *         was no "#" in the URI spec, empty string if there was a
     *         "#" but no fragment following it.
     */
     const XMLCh* getFragment() const;

    // -----------------------------------------------------------------------
    //  Setter methods
    // -----------------------------------------------------------------------

    /**
     * Set the scheme for this URI. The scheme is converted to lowercase
     * before it is set.
     *
     * @param newScheme the scheme for this URI (cannot be null)
     *
     */
     void setScheme(const XMLCh* const newScheme);

    /**
     * Set the userinfo for this URI. If a non-null value is passed in and
     * the host value is null, then an exception is thrown.
     *
     * @param newUserInfo the userinfo for this URI
     *
     */
     void setUserInfo(const XMLCh* const newUserInfo);

    /**
     * Set the host for this URI. If null is passed in, the userinfo
     * field is also set to null and the port is set to -1.
     *
     * Note: This method overwrites registry based authority if it
     * previously existed in this URI.
     *
     * @param newHost the host for this URI
     *
     */
     void setHost(const XMLCh* const newHost);

    /**
     * Set the port for this URI. -1 is used to indicate that the port is
     * not specified, otherwise valid port numbers are  between 0 and 65535.
     * If a valid port number is passed in and the host field is null,
     * an exception is thrown.
     *
     * @param newPort the port number for this URI
     *
     */
     void setPort(int newPort);
     
    /**
     * Sets the registry based authority for this URI.
     * 
     * Note: This method overwrites server based authority
     * if it previously existed in this URI.
     * 
     * @param newRegAuth the registry based authority for this URI
     */
     void setRegBasedAuthority(const XMLCh* const newRegAuth);

    /**
     * Set the path for this URI.
     *
     * If the supplied path is null, then the
     * query string and fragment are set to null as well.
     *
     * If the supplied path includes a query string and/or fragment,
     * these fields will be parsed and set as well.
     *
     * Note:
     *
     * For URIs following the "generic URI" syntax, the path
     * specified should start with a slash.
     *
     * For URIs that do not follow the generic URI syntax, this method
     * sets the scheme-specific part.
     *
     * @param newPath the path for this URI (may be null)
     *
     */
     void setPath(const XMLCh* const newPath);

    /**
     * Set the query string for this URI. A non-null value is valid only
     * if this is an URI conforming to the generic URI syntax and
     * the path value is not null.
     *
     * @param newQueryString the query string for this URI
     *
     */
     void setQueryString(const XMLCh* const newQueryString);

    /**
     * Set the fragment for this URI. A non-null value is valid only
     * if this is a URI conforming to the generic URI syntax and
     * the path value is not null.
     *
     * @param newFragment the fragment for this URI
     *
     */
     void setFragment(const XMLCh* const newFragment);

     // -----------------------------------------------------------------------
    //  Miscellaneous methods
    // -----------------------------------------------------------------------

    /**
     * Determine whether a given string contains only URI characters (also
     * called "uric" in RFC 2396). uric consist of all reserved
     * characters, unreserved characters and escaped characters.
     *
     * @return true if the string is comprised of uric, false otherwise
     */
    static bool isURIString(const XMLCh* const uric);

    /**
     * Determine whether a given string is a valid URI
     */
    static bool isValidURI( const XMLUri* const baseURI
                          , const XMLCh* const uriStr);
    /**
     * Determine whether a given string is a valid URI
     */
    static bool isValidURI( bool haveBaseURI
                          , const XMLCh* const uriStr);

    /***
     * Support for Serialization/De-serialization
     ***/
    DECL_XSERIALIZABLE(XMLUri)

    XMLUri(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

private:

    static const XMLCh MARK_OR_RESERVED_CHARACTERS[];
    static const XMLCh RESERVED_CHARACTERS[];
    static const XMLCh MARK_CHARACTERS[];
    static const XMLCh SCHEME_CHARACTERS[];
    static const XMLCh USERINFO_CHARACTERS[];
    static const XMLCh REG_NAME_CHARACTERS[];
    static const XMLCh PATH_CHARACTERS[];

    //helper method for getUriText
    void buildFullText();

    // -----------------------------------------------------------------------
    //  Private helper methods
    // -----------------------------------------------------------------------

    /**
     * Determine whether a character is a reserved character:
     *
     * @return true if the string contains any reserved characters
     */
    static bool isReservedCharacter(const XMLCh theChar);
    
    /**
     * Determine whether a character is a path character:
     *
     * @return true if the character is path character
     */
    static bool isPathCharacter(const XMLCh theChar);

    /**
     * Determine whether a char is an unreserved character.
     *
     * @return true if the char is unreserved, false otherwise
     */
    static bool isUnreservedCharacter(const XMLCh theChar);

    /**
     * Determine whether a char is an reserved or unreserved character.
     *
     * @return true if the char is reserved or unreserved, false otherwise
     */                
    static bool isReservedOrUnreservedCharacter(const XMLCh theChar);

    /**
     * Determine whether a scheme conforms to the rules for a scheme name.
     * A scheme is conformant if it starts with an alphanumeric, and
     * contains only alphanumerics, '+','-' and '.'.
     *
     * @return true if the scheme is conformant, false otherwise
     */
    static bool isConformantSchemeName(const XMLCh* const scheme);

    /**
     * Determine whether a userInfo conforms to the rules for a userinfo.
     *
     * @return true if the scheme is conformant, false otherwise
     */
    static void isConformantUserInfo(const XMLCh* const userInfo
        , MemoryManager* const manager);
    
    /**
     * Determines whether the components host, port, and user info
     * are valid as a server authority.
     *
     * @return true if the given host, port, and userinfo compose
     * a valid server authority
     */
    static bool isValidServerBasedAuthority(const XMLCh* const host
                                           , const int hostLen
                                           , const int port
                                           , const XMLCh* const userinfo
                                           , const int userLen);
                                           
    /**
     * Determines whether the components host, port, and user info
     * are valid as a server authority.
     *
     * @return true if the given host, port, and userinfo compose
     * a valid server authority
     */
    static bool isValidServerBasedAuthority(const XMLCh* const host
                                           , const int port
                                           , const XMLCh* const userinfo
                                           , MemoryManager* const manager);
      
   /**
    * Determines whether the given string is a registry based authority.
    * 
    * @param authority the authority component of a URI
    * 
    * @return true if the given string is a registry based authority
    */
    static bool isValidRegistryBasedAuthority(const XMLCh* const authority
                                             , const int authLen);

   /**
    * Determines whether the given string is a registry based authority.
    * 
    * @param authority the authority component of a URI
    * 
    * @return true if the given string is a registry based authority
    */
    static bool isValidRegistryBasedAuthority(const XMLCh* const authority);

    /**
     * Determine whether a string is syntactically capable of representing
     * a valid IPv4 address, IPv6 reference or the domain name of a network host.
     *
     * A valid IPv4 address consists of four decimal digit groups
     * separated by a '.'.
     *
     * See RFC 2732 Section 3, and RFC 2373 Section 2.2, for the 
     * definition of IPv6 references.
     *
     * A hostname consists of domain labels (each of which must begin and
     * end with an alphanumeric but may contain '-') separated by a '.'.
     * See RFC 2396 Section 3.2.2.
     *
     * @return true if the string is a syntactically valid IPv4 address
     *              or hostname
     */
     static bool isWellFormedAddress(const XMLCh* const addr
         , MemoryManager* const manager);
     
    /**
     * Determines whether a string is an IPv4 address as defined by 
     * RFC 2373, and under the further constraint that it must be a 32-bit
     * address. Though not expressed in the grammar, in order to satisfy 
     * the 32-bit address constraint, each segment of the address cannot 
     * be greater than 255 (8 bits of information).
     *
     * @return true if the string is a syntactically valid IPv4 address
     */
     static bool isWellFormedIPv4Address(const XMLCh* const addr, const int length);
     
    /**
     * Determines whether a string is an IPv6 reference as defined
     * by RFC 2732, where IPv6address is defined in RFC 2373. The 
     * IPv6 address is parsed according to Section 2.2 of RFC 2373,
     * with the additional constraint that the address be composed of
     * 128 bits of information.
     *
     * Note: The BNF expressed in RFC 2373 Appendix B does not 
     * accurately describe section 2.2, and was in fact removed from
     * RFC 3513, the successor of RFC 2373.
     *
     * @return true if the string is a syntactically valid IPv6 reference
     */
     static bool isWellFormedIPv6Reference(const XMLCh* const addr, const int length);
     
    /**
     * Helper function for isWellFormedIPv6Reference which scans the 
     * hex sequences of an IPv6 address. It returns the index of the 
     * next character to scan in the address, or -1 if the string 
     * cannot match a valid IPv6 address. 
     *
     * @param address the string to be scanned
     * @param index the beginning index (inclusive)
     * @param end the ending index (exclusive)
     * @param counter a counter for the number of 16-bit sections read
     * in the address
     *
     * @return the index of the next character to scan, or -1 if the
     * string cannot match a valid IPv6 address
     */
     static int scanHexSequence (const XMLCh* const addr, int index, int end, int& counter);

    /**
     * Get the indicator as to whether this URI uses the "generic URI"
     * syntax.
     *
     * @return true if this URI uses the "generic URI" syntax, false
     *         otherwise
     */
     bool isGenericURI();

    // -----------------------------------------------------------------------
    //  Miscellaneous methods
    // -----------------------------------------------------------------------

    /**
     * Initialize all fields of this URI from another URI.
     *
     * @param toCopy the URI to copy (cannot be null)
     */
     void initialize(const XMLUri& toCopy);

    /**
     * Initializes this URI from a base URI and a URI specification string.
     * See RFC 2396 Section 4 and Appendix B for specifications on parsing
     * the URI and Section 5 for specifications on resolving relative URIs
     * and relative paths.
     *
     * @param baseURI the base URI (may be null if uriSpec is an absolute
     *               URI)
     *
     * @param uriSpec the URI spec string which may be an absolute or
     *                  relative URI (can only be null/empty if base
     *                  is not null)
     *
     */
     void initialize(const XMLUri* const baseURI
                   , const XMLCh*  const uriSpec);

    /**
     * Initialize the scheme for this URI from a URI string spec.
     *
     * @param uriSpec the URI specification (cannot be null)
     *
     */
     void initializeScheme(const XMLCh* const uriSpec);

    /**
     * Initialize the authority (userinfo, host and port) for this
     * URI from a URI string spec.
     *
     * @param uriSpec the URI specification (cannot be null)
     *
     */
     void initializeAuthority(const XMLCh* const uriSpec);

    /**
     * Initialize the path for this URI from a URI string spec.
     *
     * @param uriSpec the URI specification (cannot be null)
     *
     */
     void initializePath(const XMLCh* const uriSpec);

     /**
      * cleanup the data variables
      *
      */
     void cleanUp();

    static bool isConformantSchemeName(const XMLCh* const scheme,
                                       const int schemeLen);
    static bool processScheme(const XMLCh* const uriStr, int& index);
    static bool processAuthority(const XMLCh* const uriStr, const int authLen);
    static bool isWellFormedAddress(const XMLCh* const addr, const int addrLen);
    static bool processPath(const XMLCh* const pathStr, const int pathStrLen,
                            const bool isSchemePresent);

    // -----------------------------------------------------------------------
    //  Data members
    //
    //  for all the data member, we own it,
    //  responsible for the creation and/or deletion for
    //  the memory allocated.
    //
    // -----------------------------------------------------------------------
    XMLCh*          fScheme;
    XMLCh*          fUserInfo;
    XMLCh*          fHost;
    int             fPort;
    XMLCh*          fRegAuth;
    XMLCh*          fPath;
    XMLCh*          fQueryString;
    XMLCh*          fFragment;
    XMLCh*          fURIText;
    MemoryManager*  fMemoryManager;
};

// ---------------------------------------------------------------------------
//  XMLUri: Getter methods
// ---------------------------------------------------------------------------
inline const XMLCh* XMLUri::getScheme() const
{
    return fScheme;
}

inline const XMLCh* XMLUri::getUserInfo() const
{
	return fUserInfo;
}

inline const XMLCh* XMLUri::getHost() const
{
	return fHost;
}

inline int XMLUri::getPort() const
{
	return fPort;
}

inline const XMLCh* XMLUri::getRegBasedAuthority() const
{
	return fRegAuth;
}

inline const XMLCh* XMLUri::getPath() const
{
	return fPath;
}

inline const XMLCh* XMLUri::getQueryString() const
{
	return fQueryString;
}

inline const XMLCh* XMLUri::getFragment() const
{
	return fFragment;
}

inline const XMLCh* XMLUri::getUriText() const
{
    //
    //  Fault it in if not already. Since this is a const method and we
    //  can't use mutable members due the compilers we have to support,
    //  we have to cast off the constness.
    //
    if (!fURIText)
        ((XMLUri*)this)->buildFullText();

    return fURIText;
}

// ---------------------------------------------------------------------------
//  XMLUri: Helper methods
// ---------------------------------------------------------------------------
inline bool XMLUri::isReservedOrUnreservedCharacter(const XMLCh theChar)
{
   return (XMLString::isAlphaNum(theChar) ||
           XMLString::indexOf(MARK_OR_RESERVED_CHARACTERS, theChar) != -1);
}

inline bool XMLUri::isReservedCharacter(const XMLCh theChar)
{
    return (XMLString::indexOf(RESERVED_CHARACTERS, theChar) != -1);
}

inline bool XMLUri::isPathCharacter(const XMLCh theChar)
{
    return (XMLString::indexOf(PATH_CHARACTERS, theChar) != -1);
}

inline bool XMLUri::isUnreservedCharacter(const XMLCh theChar)
{
    return (XMLString::isAlphaNum(theChar) ||
            XMLString::indexOf(MARK_CHARACTERS, theChar) != -1);
}

XERCES_CPP_NAMESPACE_END

#endif
