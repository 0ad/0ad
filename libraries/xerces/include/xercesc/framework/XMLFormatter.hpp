/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 1999-2000 The Apache Software Foundation.  All rights
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
 * $Log: XMLFormatter.hpp,v $
 * Revision 1.20  2004/01/29 11:46:29  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.19  2003/12/01 23:23:25  neilg
 * fix for bug 25118; thanks to Jeroen Witmond
 *
 * Revision 1.18  2003/10/10 02:06:09  neilg
 * fix for bug 21780; thanks to David Cargill
 *
 * Revision 1.17  2003/09/22 08:50:04  gareth
 * doc fix
 *
 * Revision 1.16  2003/09/08 21:48:36  peiyongz
 * Restore pre2.3 constructors
 *
 * Revision 1.15  2003/05/30 16:11:43  gareth
 * Fixes so we compile under VC7.1. Patch by Alberto Massari.
 *
 * Revision 1.14  2003/05/16 21:36:55  knoaman
 * Memory manager implementation: Modify constructors to pass in the memory manager.
 *
 * Revision 1.13  2003/05/15 18:26:07  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.12  2003/03/17 03:19:52  peiyongz
 * Bug#18051 memory leakage in XMLFormatter
 *
 * Revision 1.11  2003/03/16 06:00:43  peiyongz
 * Bug#17983 Formatter does not escape control characters
 *
 * Revision 1.10  2003/03/11 12:58:36  tng
 * Fix compilation error on AIX.
 *
 * Revision 1.9  2003/03/07 21:42:37  tng
 * [Bug 17589] Refactoring ... .  Patch from Jacques Legare.
 *
 * Revision 1.8  2003/03/07 18:08:10  tng
 * Return a reference instead of void for operator=
 *
 * Revision 1.7  2003/01/31 00:30:48  jberry
 * Syntax error in declaration
 *
 * Revision 1.6  2003/01/28 18:32:33  peiyongz
 * Bug#13694: Allow Xerces to write the BOM to XML files
 *
 * Revision 1.5  2003/01/24 20:20:22  tng
 * Add method flush to XMLFormatTarget
 *
 * Revision 1.4  2002/11/04 15:00:21  tng
 * C++ Namespace Support.
 *
 * Revision 1.3  2002/07/30 16:29:16  tng
 * [Bug 8550] No explanation of XMLFormatter escape options.
 *
 * Revision 1.2  2002/06/21 19:31:23  peiyongz
 * getTranscoder() added;
 *
 * Revision 1.1.1.1  2002/02/01 22:21:52  peiyongz
 * sane_include
 *
 * Revision 1.7  2000/10/17 19:25:38  andyh
 * XMLFormatTarget, removed version of writeChars with no length.  Can not be
 * safely used, and obscured other errors.
 *
 * Revision 1.6  2000/10/10 23:54:58  andyh
 * XMLFormatter patch, contributed by Bill Schindler.  Fix problems with
 * output to multi-byte encodings.
 *
 * Revision 1.5  2000/04/07 01:01:56  roddey
 * Fixed an error message so that it indicated the correct radix for the rep
 * token. Get all of the basic output formatting functionality in place for
 * at least ICU and Win32 transcoders.
 *
 * Revision 1.4  2000/04/06 23:50:38  roddey
 * Now the low level formatter handles doing char refs for
 * unrepresentable chars (in addition to the replacement char style
 * already done.)
 *
 * Revision 1.3  2000/04/06 19:09:21  roddey
 * Some more improvements to output formatting. Now it will correctly
 * handle doing the 'replacement char' style of dealing with chars
 * that are unrepresentable.
 *
 * Revision 1.2  2000/04/05 00:20:16  roddey
 * More updates for the low level formatted output support
 *
 * Revision 1.1  2000/03/28 19:43:17  roddey
 * Fixes for signed/unsigned warnings. New work for two way transcoding
 * stuff.
 *
 */

#if !defined(XMLFORMATTER_HPP)
#define XMLFORMATTER_HPP

#include <xercesc/util/PlatformUtils.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class XMLFormatTarget;
class XMLTranscoder;

/**
 *  This class provides the basic formatting capabilities that are required
 *  to turn the Unicode based XML data from the parsers into a form that can
 *  be used on non-Unicode based systems, that is, into local or generic text
 *  encodings.
 *
 *  A number of flags are provided to control whether various optional
 *  formatting operations are performed.
 */
class XMLPARSER_EXPORT XMLFormatter : public XMemory
{
public:
    // -----------------------------------------------------------------------
    //  Class types
    // -----------------------------------------------------------------------
    /** @name Public Contants */
    //@{
    /**
     * EscapeFlags - Different styles of escape flags to control various formatting.
     *
     * <p><code>NoEscapes:</code>
     * No character needs to be escaped.   Just write them out as is.</p>
     * <p><code>StdEscapes:</code>
     * The following characters need to be escaped:</p>
     * <table border='1'>
     * <tr>
     * <td>character</td>
     * <td>should be escaped and written as</td>
     * </tr>
     * <tr>
     * <td valign='top' rowspan='1' colspan='1'>&amp;</td>
     * <td valign='top' rowspan='1' colspan='1'>&amp;amp;</td>
     * </tr>
     * <tr>
     * <td valign='top' rowspan='1' colspan='1'>&gt;</td>
     * <td valign='top' rowspan='1' colspan='1'>&amp;gt;</td>
     * </tr>
     * <tr>
     * <td valign='top' rowspan='1' colspan='1'>&quot;</td>
     * <td valign='top' rowspan='1' colspan='1'>&amp;quot;</td>
     * </tr>
     * <tr>
     * <td valign='top' rowspan='1' colspan='1'>&lt;</td>
     * <td valign='top' rowspan='1' colspan='1'>&amp;lt;</td>
     * </tr>
     * <tr>
     * <td valign='top' rowspan='1' colspan='1'>&apos;</td>
     * <td valign='top' rowspan='1' colspan='1'>&amp;apos;</td>
     * </tr>
     * </table>
     * <p><code>AttrEscapes:</code>
     * The following characters need to be escaped:</p>
     * <table border='1'>
     * <tr>
     * <td>character</td>
     * <td>should be escaped and written as</td>
     * </tr>
     * <tr>
     * <td valign='top' rowspan='1' colspan='1'>&amp;</td>
     * <td valign='top' rowspan='1' colspan='1'>&amp;amp;</td>
     * </tr>
     * <tr>
     * <td valign='top' rowspan='1' colspan='1'>&gt;</td>
     * <td valign='top' rowspan='1' colspan='1'>&amp;gt;</td>
     * </tr>
     * <tr>
     * <td valign='top' rowspan='1' colspan='1'>&quot;</td>
     * <td valign='top' rowspan='1' colspan='1'>&amp;quot;</td>
     * </tr>
     * </table>
     * <p><code>CharEscapes:</code>
     * The following characters need to be escaped:</p>
     * <table border='1'>
     * <tr>
     * <td>character</td>
     * <td>should be escaped and written as</td>
     * </tr>
     * <tr>
     * <td valign='top' rowspan='1' colspan='1'>&amp;</td>
     * <td valign='top' rowspan='1' colspan='1'>&amp;amp;</td>
     * </tr>
     * <tr>
     * <td valign='top' rowspan='1' colspan='1'>&gt;</td>
     * <td valign='top' rowspan='1' colspan='1'>&amp;gt;</td>
     * </tr>
     * </table>
     * <p><code>EscapeFlags_Count:</code>
     * Special value, do not use directly.</p>
     * <p><code>DefaultEscape:</code>
     * Special value, do not use directly.</p>
     *
     */
    enum EscapeFlags
    {
        NoEscapes
        , StdEscapes
        , AttrEscapes
        , CharEscapes

        // Special values, don't use directly
        , EscapeFlags_Count
        , DefaultEscape     = 999
    };

    /**
     * UnRepFlags
     *
     * The unrepresentable flags that indicate how to react when a
     * character cannot be represented in the target encoding.
     *
     * <p><code>UnRep_Fail:</code>
     * Fail the operation.</p>
     * <p><code>UnRep_CharRef:</code>
     * Display the unrepresented character as reference.</p>
     * <p><code>UnRep_Replace:</code>
     * Replace the unrepresented character with the replacement character.</p>
     * <p><code>DefaultUnRep:</code>
     * Special value, do not use directly.</p>
     *
     */
    enum UnRepFlags
    {
        UnRep_Fail
        , UnRep_CharRef
        , UnRep_Replace

        , DefaultUnRep      = 999
    };
    //@}


    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    /** @name Constructor and Destructor */
    //@{
    /**
     * @param outEncoding the encoding for the formatted content
     * @param docVersion  
     * @param target      the formatTarget where the formatted content is written to
     * @param escapeFlags the escape style for certain character
     * @param unrepFlags  the reaction to unrepresentable character
     * @param manager     Pointer to the memory manager to be used to
     *                    allocate objects.
     */
    XMLFormatter
    (
        const   XMLCh* const            outEncoding
        , const XMLCh* const            docVersion
        ,       XMLFormatTarget* const  target
        , const EscapeFlags             escapeFlags = NoEscapes
        , const UnRepFlags              unrepFlags = UnRep_Fail
        ,       MemoryManager* const    manager = XMLPlatformUtils::fgMemoryManager
    );

    XMLFormatter
    (
        const   char* const             outEncoding
        , const char* const             docVersion
        ,       XMLFormatTarget* const  target
        , const EscapeFlags             escapeFlags = NoEscapes
        , const UnRepFlags              unrepFlags = UnRep_Fail
        ,       MemoryManager* const    manager = XMLPlatformUtils::fgMemoryManager
    );

    XMLFormatter
    (
        const   XMLCh* const            outEncoding
        ,       XMLFormatTarget* const  target
        , const EscapeFlags             escapeFlags = NoEscapes
        , const UnRepFlags              unrepFlags = UnRep_Fail
        ,       MemoryManager* const    manager = XMLPlatformUtils::fgMemoryManager
    );

    XMLFormatter
    (
        const   char* const             outEncoding
        ,       XMLFormatTarget* const  target
        , const EscapeFlags             escapeFlags = NoEscapes
        , const UnRepFlags              unrepFlags = UnRep_Fail
        ,       MemoryManager* const    manager = XMLPlatformUtils::fgMemoryManager
    );
    
    ~XMLFormatter();
    //@}


    // -----------------------------------------------------------------------
    //  Formatting methods
    // -----------------------------------------------------------------------
    /** @name Formatting methods */
    //@{
    /**
     * @param toFormat the string to be formatted
     * @param count    length of the string
     * @param escapeFlags the escape style for formatting toFormat
     * @param unrepFlags the reaction for any unrepresentable character in toFormat
     *
     */
    void formatBuf
    (
        const   XMLCh* const    toFormat
        , const unsigned int    count
        , const EscapeFlags     escapeFlags = DefaultEscape
        , const UnRepFlags      unrepFlags = DefaultUnRep
    );

    /**
     * @see formatBuf
     */
    XMLFormatter& operator<<
    (
        const   XMLCh* const    toFormat
    );

    XMLFormatter& operator<<
    (
        const   XMLCh           toFormat
    );

    void writeBOM(const XMLByte* const toFormat
                , const unsigned int   count);

    //@}

    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    /** @name Getter methods */
    //@{
    /**
     * @return return the encoding set for the formatted content
     */

    const XMLCh* getEncodingName() const;

    /**
     * @return return the transcoder used internally for transcoding the formatter conent
     */
    inline const XMLTranscoder*   getTranscoder() const;

   //@}

    // -----------------------------------------------------------------------
    //  Setter methods
    // -----------------------------------------------------------------------
    /** @name Setter methods */
    //@{
    /**
     * @param newFlags set the escape style for the follow-on formatted content
     */
    void setEscapeFlags
    (
        const   EscapeFlags     newFlags
    );

    /**
     * @param newFlags set the reaction for unrepresentable character
     */
    void setUnRepFlags
    (
        const   UnRepFlags      newFlags
    );

    /**
     * @param newFlags set the escape style for the follow-on formatted content
     * @see setEscapeFlags
     */
    XMLFormatter& operator<<
    (
        const   EscapeFlags     newFlags
    );

    /**
     * @param newFlags set the reaction for unrepresentable character
     * @see setUnRepFlags
     */
    XMLFormatter& operator<<
    (
        const   UnRepFlags      newFlags
    );
    //@}


private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    XMLFormatter();
    XMLFormatter(const XMLFormatter&);
    XMLFormatter& operator=(const XMLFormatter&);


    // -----------------------------------------------------------------------
    //  Private class constants
    // -----------------------------------------------------------------------
    enum Constants
    {
        kTmpBufSize     = 16 * 1024
    };


    // -----------------------------------------------------------------------
    //  Private helper methods
    // -----------------------------------------------------------------------
    const XMLByte* getCharRef(unsigned int & count, 
                              XMLByte*      &ref, 
                              const XMLCh *  stdRef);  
 
    void writeCharRef(const XMLCh &toWrite);
    void writeCharRef(unsigned long toWrite);

    bool inEscapeList(const XMLFormatter::EscapeFlags escStyle
                    , const XMLCh                     toCheck);
                              

    unsigned int handleUnEscapedChars(const XMLCh *                  srcPtr, 
                                      const unsigned int             count, 
                                      const UnRepFlags               unrepFlags);

    void specialFormat
    (
        const   XMLCh* const    toFormat
        , const unsigned int    count
        , const EscapeFlags     escapeFlags
    );


    // -----------------------------------------------------------------------
    //  Private, non-virtual methods
    //
    //  fEscapeFlags
    //      The escape flags we were told to use in formatting. These are
    //      defaults set in the ctor, which can be overridden on a particular
    //      call.
    //
    //  fOutEncoding
    //      This the name of the output encoding. Saved mainly for meaningful
    //      error messages.
    //
    //  fTarget
    //      This is the target object for the formatting operation.
    //
    //  fUnRepFlags
    //      The unrepresentable flags that indicate how to react when a
    //      character cannot be represented in the target encoding.
    //
    //  fXCoder
    //      This the transcoder that we will use. It is created using the
    //      encoding name we were told to use.
    //
    //  fTmpBuf
    //      An output buffer that we use to transcode chars into before we
    //      send them off to be output.
    //
    //  fAposRef
    //  fAmpRef
    //  fGTRef
    //  fLTRef
    //  fQuoteRef
    //      These are character refs for the standard char refs, in the
    //      output encoding. They are faulted in as required, by transcoding
    //      them from fixed Unicode versions.
    //
    //  fIsXML11
    //      for performance reason, we do not store the actual version string
    //      and do the string comparison again and again.
    //
    // -----------------------------------------------------------------------
    EscapeFlags                 fEscapeFlags;
    XMLCh*                      fOutEncoding;
    XMLFormatTarget*            fTarget;
    UnRepFlags                  fUnRepFlags;
    XMLTranscoder*              fXCoder;
    XMLByte                     fTmpBuf[kTmpBufSize + 4];
    XMLByte*                    fAposRef;
    unsigned int                fAposLen;
    XMLByte*                    fAmpRef;
    unsigned int                fAmpLen;
    XMLByte*                    fGTRef;
    unsigned int                fGTLen;
    XMLByte*                    fLTRef;
    unsigned int                fLTLen;
    XMLByte*                    fQuoteRef;
    unsigned int                fQuoteLen;
    bool                        fIsXML11;
    MemoryManager*              fMemoryManager;
};


class XMLPARSER_EXPORT XMLFormatTarget : public XMemory
{
public:
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    virtual ~XMLFormatTarget() {}


    // -----------------------------------------------------------------------
    //  Virtual interface
    // -----------------------------------------------------------------------
    virtual void writeChars
    (
          const XMLByte* const      toWrite
        , const unsigned int        count
        ,       XMLFormatter* const formatter
    ) = 0;

    virtual void flush() {};


protected :
    // -----------------------------------------------------------------------
    //  Hidden constructors and operators
    // -----------------------------------------------------------------------
    XMLFormatTarget() {};

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    XMLFormatTarget(const XMLFormatTarget&);
    XMLFormatTarget& operator=(const XMLFormatTarget&);
};


// ---------------------------------------------------------------------------
//  XMLFormatter: Getter methods
// ---------------------------------------------------------------------------
inline const XMLCh* XMLFormatter::getEncodingName() const
{
    return fOutEncoding;
}

inline const XMLTranscoder* XMLFormatter::getTranscoder() const
{
    return fXCoder;
}

// ---------------------------------------------------------------------------
//  XMLFormatter: Setter methods
// ---------------------------------------------------------------------------
inline void XMLFormatter::setEscapeFlags(const EscapeFlags newFlags)
{
    fEscapeFlags = newFlags;
}

inline void XMLFormatter::setUnRepFlags(const UnRepFlags newFlags)
{
    fUnRepFlags = newFlags;
}


inline XMLFormatter& XMLFormatter::operator<<(const EscapeFlags newFlags)
{
    fEscapeFlags = newFlags;
    return *this;
}

inline XMLFormatter& XMLFormatter::operator<<(const UnRepFlags newFlags)
{
    fUnRepFlags = newFlags;
    return *this;
}

XERCES_CPP_NAMESPACE_END

#endif
