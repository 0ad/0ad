/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 1999-2003 The Apache Software Foundation.  All rights
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
  * $Log: XMLValidator.hpp,v $
  * Revision 1.9  2003/05/15 18:26:07  knoaman
  * Partial implementation of the configurable memory manager.
  *
  * Revision 1.8  2003/03/07 18:08:10  tng
  * Return a reference instead of void for operator=
  *
  * Revision 1.7  2002/11/07 21:59:22  tng
  * Pass elemDecl to XMLValidator::validateAttrValue so that we can include element name in error message.
  *
  * Revision 1.6  2002/11/04 15:00:21  tng
  * C++ Namespace Support.
  *
  * Revision 1.5  2002/09/04 18:17:55  tng
  * Do not set IDREF to used during prevalidation.
  *
  * Revision 1.4  2002/08/20 16:54:11  tng
  * [Bug 6251] Info during compilation.
  *
  * Revision 1.3  2002/07/11 18:55:44  knoaman
  * Add a flag to the preContentValidation method to indicate whether to validate
  * default/fixed attributes or not.
  *
  * Revision 1.2  2002/02/20 18:17:01  tng
  * [Bug 5977] Warnings on generating apiDocs.
  *
  * Revision 1.1.1.1  2002/02/01 22:21:52  peiyongz
  * sane_include
  *
  * Revision 1.17  2001/11/30 22:18:18  peiyongz
  * cleanUp function made member function
  * cleanUp object moved to file scope
  * double mutex lock removed
  *
  * Revision 1.16  2001/11/13 13:24:46  tng
  * Deprecate function XMLValidator::checkRootElement.
  *
  * Revision 1.15  2001/10/24 23:46:52  peiyongz
  * [Bug 4342] fix the leak.
  *
  * Revision 1.14  2001/06/05 16:51:17  knoaman
  * Add 'const' to getGrammar - submitted by Peter A. Volchek.
  *
  * Revision 1.13  2001/05/11 13:25:33  tng
  * Copyright update.
  *
  * Revision 1.12  2001/05/03 20:34:22  tng
  * Schema: SchemaValidator update
  *
  * Revision 1.11  2001/04/19 18:16:53  tng
  * Schema: SchemaValidator update, and use QName in Content Model
  *
  * Revision 1.10  2001/03/21 21:56:03  tng
  * Schema: Add Schema Grammar, Schema Validator, and split the DTDValidator into DTDValidator, DTDScanner, and DTDGrammar.
  *
  * Revision 1.9  2001/02/26 19:21:33  tng
  * Schema: add parameter prefix in findElem and findAttr.
  *
  * Revision 1.8  2000/12/14 18:49:58  tng
  * Fix API document generation warning: "Warning: end of member group without matching begin"
  *
  * Revision 1.7  2000/08/09 22:09:09  jpolast
  * added const XMLCh* getURIText()
  * allows parsers to use const URIs instead of appending
  * to a XMLBuffer.
  *
  * Revision 1.6  2000/03/02 19:54:25  roddey
  * This checkin includes many changes done while waiting for the
  * 1.1.0 code to be finished. I can't list them all here, but a list is
  * available elsewhere.
  *
  * Revision 1.5  2000/02/24 20:00:24  abagchi
  * Swat for removing Log from API docs
  *
  * Revision 1.4  2000/02/15 23:59:07  roddey
  * More updated documentation of Framework classes.
  *
  * Revision 1.3  2000/02/15 01:21:31  roddey
  * Some initial documentation improvements. More to come...
  *
  * Revision 1.2  2000/02/06 07:47:49  rahulj
  * Year 2K copyright swat.
  *
  * Revision 1.1.1.1  1999/11/09 01:08:38  twl
  * Initial checkin
  *
  * Revision 1.4  1999/11/08 20:44:41  rahul
  * Swat for adding in Product name and CVS comment log variable.
  *
  */


#if !defined(XMLVALIDATOR_HPP)
#define XMLVALIDATOR_HPP

#include <xercesc/framework/XMLAttr.hpp>
#include <xercesc/framework/XMLValidityCodes.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class ReaderMgr;
class XMLBufferMgr;
class XMLElementDecl;
class XMLScanner;
class Grammar;


/**
 *  This abstract class provides the interface for all validators. This is
 *  the simple amount of API that all validators must honor, in order for
 *  the scanner to use them to do validation. All validators will actually
 *  contain much more functionality than is accessible via this common API,
 *  but that functionality requires that you know what type of validator you
 *  are dealing with.
 *
 *  Basically, at this level, the primary concern is to be able to query
 *  core information about elements and attributes. Adding decls to the
 *  validator requires that you go through the derived interface because they
 *  all have their own decl types. At this level, we can return information
 *  via the base decl classes, from which each validator derives its own
 *  decl classes.
 */
class XMLPARSER_EXPORT XMLValidator : public XMemory
{
public:
    // -----------------------------------------------------------------------
    //  Constructors are hidden, just the virtual destructor is exposed
    // -----------------------------------------------------------------------

    /** @name Destructor */
    //@{

    /**
     *  The derived class should clean up its allocated data, then this class
     *  will do the same for data allocated at this level.
     */
    virtual ~XMLValidator()
    {
    }
    //@}


    // -----------------------------------------------------------------------
    //  The virtual validator interface
    // -----------------------------------------------------------------------

    /** @name Virtual validator interface */
    //@{

    /**
      * The derived class should look up its declaration of the passed element
      * from its element pool. It should then use the content model description
      * contained in that element declaration to validate that the passed list
      * of child elements are valid for that content model. The count can be
      * zero, indicating no child elements.
      *
      * Note that whitespace and text content are not validated here. Those are
      * handled by the scanner. So only element ids are provided here.
      *
      * @param  elemDecl    The element whose content is to be checked.
      *
      * @param  children    An array of element QName which represent the elements
      *                     found within the parent element, i.e. the content
      *                     to be validated.
      *
      * @param  childCount  The number of elements in the childIds array. It can
      *                     be zero if the element had none.
      */
    virtual int checkContent
    (
        XMLElementDecl* const   elemDecl
        , QName** const         children
        , const unsigned int    childCount
    ) = 0;

    /**
      * The derived class should fault in the passed XMLAttr value. It should
      * use the passeed attribute definition (which is passed via the base
      * type so it must often be downcast to the appropriate type for the
      * derived validator class), to fill in the passed attribute. This is done
      * as a performance enhancement since the derived class has more direct
      * access to the information.
      */
    virtual void faultInAttr
    (
                XMLAttr&    toFill
        , const XMLAttDef&  attDef
    )   const = 0;

    /**
      * This method is called by the scanner after a Grammar is scanned.
      */
    virtual void preContentValidation(bool reuseGrammar,
                                      bool validateDefAttr = false) = 0;

    /**
      * This method is called by the scanner after the parse has completed. It
      * gives the validator a chance to check certain things that can only be
      * checked after the whole document has been parsed, such as referential
      * integrity of ID/IDREF pairs and so forth. The validator should just
      * issue errors for any problems it finds.
      */
    virtual void postParseValidation() = 0;

    /**
      * This method is called by the scanner before a new document is about
      * to start. It gives the validator a change to reset itself in preperation
      * for another validation pass.
      */
    virtual void reset() = 0;

    /**
      * The derived class should return a boolean that indicates whether it
      * requires namespace processing or not. Some do and some allow it to be
      * optional. This flag is used to control whether the client code's
      * requests to disable namespace processing can be honored or not.
      */
    virtual bool requiresNamespaces() const = 0;

    /**
      * The derived class should apply any rules to the passed attribute value
      * that are above and beyond those defined by XML 1.0. The scanner itself
      * will impose XML 1.0 rules, based on the type of the attribute. This
      * will generally be used to check things such as range checks and other
      * datatype related validation.
      *
      * If the value breaks any rules as defined by the derived class, it
      * should just issue errors as usual.
      */
    virtual void validateAttrValue
    (
        const   XMLAttDef*                  attDef
        , const XMLCh* const                attrValue
        , bool                              preValidation = false
        , const XMLElementDecl*             elemDecl = 0
    ) = 0;

    /**
      * The derived class should apply any rules to the passed element decl
      * that are above and beyond those defined by XML 1.0.
      *
      * If the value breaks any rules as defined by the derived class, it
      * should just issue errors as usual.
      */
    virtual void validateElement
    (
        const   XMLElementDecl*             elemDef
    ) = 0;

    /**
      * Retrieve the Grammar used
      */
    virtual Grammar* getGrammar() const =0;

    /**
      * Set the Grammar
      */
    virtual void setGrammar(Grammar* aGrammar) =0;


    //@}

    // -----------------------------------------------------------------------
    //  Virtual DTD handler interface.
    // -----------------------------------------------------------------------

    /** @name Virtual DTD handler interface */
    //@{

    /**
      * This method allows the scanner to ask the validator if it handles
      * DTDs or not.
      */
    virtual bool handlesDTD() const = 0;

    // -----------------------------------------------------------------------
    //  Virtual Schema handler interface.
    // -----------------------------------------------------------------------

    /** @name Virtual Schema handler interface */

    /**
      * This method allows the scanner to ask the validator if it handles
      * Schema or not.
      */
    virtual bool handlesSchema() const = 0;

    //@}

    // -----------------------------------------------------------------------
    //  Setter methods
    //
    //  setScannerInfo() is called by the scanner to tell the validator
    //  about the stuff it needs to have access to.
    // -----------------------------------------------------------------------

    /** @name Setter methods */
    //@{

    /**
      * @param  owningScanner   This is a pointer to the scanner to which the
      *                         validator belongs. The validator will often
      *                         need to query state data from the scanner.
      *
      * @param  readerMgr       This is a pointer to the reader manager that is
      *                         being used by the scanner.
      *
      * @param  bufMgr          This is the buffer manager of the scanner. This
      *                         is provided as a convenience so that the validator
      *                         doesn't have to create its own buffer manager
      *                         during the parse process.
      */
    void setScannerInfo
    (
        XMLScanner* const           owningScanner
        , ReaderMgr* const          readerMgr
        , XMLBufferMgr* const       bufMgr
    );

    /**
      * This method is called to set an error reporter on the validator via
      * which it will report any errors it sees during parsing or validation.
      * This is generally called by the owning scanner.
      *
      * @param  errorReporter   A pointer to the error reporter to use. This
      *                         is not adopted, just referenced so the caller
      *                         remains responsible for its cleanup, if any.
      */
    void setErrorReporter
    (
        XMLErrorReporter* const errorReporter
    );

    //@}


    // -----------------------------------------------------------------------
    //  Error emitter methods
    // -----------------------------------------------------------------------

    /** @name Error emittor methods */
    //@{

    /**
     *  This call is a convenience by which validators can emit errors. Most
     *  of the grunt work of loading the text, getting the current source
     *  location, ect... is handled here.
     *
     *  If the loaded text has replacement parameters, then text strings can be
     *  passed. These will be used to replace the tokens {0}, {1}, {2}, and {3}
     *  in the order passed. So text1 will replace {0}, text2 will replace {1},
     *  and so forth.
     *
     *  textX   Up to four replacement parameters. They can be provided
     *          as either XMLCh strings, or local code page strings which
     *          will be transcoded internally.
     *
     *  @param toEmit   The error code to emit. it must be one of the defined
     *                  validator error codes.
     *
     */
    void emitError(const XMLValid::Codes toEmit);
    void emitError
    (
        const   XMLValid::Codes toEmit
        , const XMLCh* const    text1
        , const XMLCh* const    text2 = 0
        , const XMLCh* const    text3 = 0
        , const XMLCh* const    text4 = 0
    );
    void emitError
    (
        const   XMLValid::Codes toEmit
        , const char* const     text1
        , const char* const     text2 = 0
        , const char* const     text3 = 0
        , const char* const     text4 = 0
    );

    //@}

    // -----------------------------------------------------------------------
    //  Deprecated XMLValidator interface
    // -----------------------------------------------------------------------
    /**
      *
      * DEPRECATED.
      * For those validators that contrain the possible root elements of a
      * document to only particular elements, they should use this call to
      * validate that the passed root element id is a legal root element.
      */
    bool checkRootElement
    (
        const   unsigned int
    ) { return true;};

    // -----------------------------------------------------------------------
    //  Notification that lazy data has been deleted
    // -----------------------------------------------------------------------
	static void reinitMsgMutex();

	static void reinitMsgLoader();

protected :
    // -----------------------------------------------------------------------
    //  Hidden constructors
    // -----------------------------------------------------------------------
    XMLValidator
    (
        XMLErrorReporter* const errReporter = 0
    );


    // -----------------------------------------------------------------------
    //  Protected getters
    // -----------------------------------------------------------------------
    const XMLBufferMgr* getBufMgr() const;
    XMLBufferMgr* getBufMgr();
    const ReaderMgr* getReaderMgr() const;
    ReaderMgr* getReaderMgr();
    const XMLScanner* getScanner() const;
    XMLScanner* getScanner();


private :
    // -----------------------------------------------------------------------
    //  Unimplemented Constructors and Operators
    // -----------------------------------------------------------------------
    XMLValidator(const XMLValidator&);
    XMLValidator& operator=(const XMLValidator&);


    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fErrorReporter
    //      The error reporter we are to use, if any.
    //
    // -----------------------------------------------------------------------
    XMLBufferMgr*       fBufMgr;
    XMLErrorReporter*   fErrorReporter;
    ReaderMgr*          fReaderMgr;
    XMLScanner*         fScanner;
};


// -----------------------------------------------------------------------
//  Setter methods
// -----------------------------------------------------------------------
inline void
XMLValidator::setScannerInfo(XMLScanner* const      owningScanner
                            , ReaderMgr* const      readerMgr
                            , XMLBufferMgr* const   bufMgr)
{
    // We don't own any of these, we just reference them
    fScanner = owningScanner;
    fReaderMgr = readerMgr;
    fBufMgr = bufMgr;
}

inline void
XMLValidator::setErrorReporter(XMLErrorReporter* const errorReporter)
{
    fErrorReporter = errorReporter;
}


// ---------------------------------------------------------------------------
//  XMLValidator: Protected getter
// ---------------------------------------------------------------------------
inline const XMLBufferMgr* XMLValidator::getBufMgr() const
{
    return fBufMgr;
}

inline XMLBufferMgr* XMLValidator::getBufMgr()
{
    return fBufMgr;
}

inline const ReaderMgr* XMLValidator::getReaderMgr() const
{
    return fReaderMgr;
}

inline ReaderMgr* XMLValidator::getReaderMgr()
{
    return fReaderMgr;
}

inline const XMLScanner* XMLValidator::getScanner() const
{
    return fScanner;
}

inline XMLScanner* XMLValidator::getScanner()
{
    return fScanner;
}

XERCES_CPP_NAMESPACE_END

#endif
