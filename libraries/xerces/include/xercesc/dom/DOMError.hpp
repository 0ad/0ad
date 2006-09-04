#ifndef DOMError_HEADER_GUARD_
#define DOMError_HEADER_GUARD_

/*
 * Copyright 2002,2004 The Apache Software Foundation.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * $Id: DOMError.hpp 191054 2005-06-17 02:56:35Z jberry $
 */


#include <xercesc/util/XercesDefs.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class DOMLocator;


/**
  * DOMError is an interface that describes an error.
  *
  * @see DOMErrorHandler#handleError
  * @since DOM Level 3
  */

class CDOM_EXPORT DOMError
{
protected:
    // -----------------------------------------------------------------------
    //  Hidden constructors
    // -----------------------------------------------------------------------
    /** @name Hidden constructors */
    //@{    
    DOMError() {};
    //@}

private:
    // -----------------------------------------------------------------------
    // Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    /** @name Unimplemented constructors and operators */
    //@{
    DOMError(const DOMError &);
    DOMError & operator = (const DOMError &);
    //@}

public:
    // -----------------------------------------------------------------------
    //  All constructors are hidden, just the destructor is available
    // -----------------------------------------------------------------------
    /** @name Destructor */
    //@{
    /**
     * Destructor
     *
     */
    virtual ~DOMError() {};
    //@}

    // -----------------------------------------------------------------------
    //  Class types
    // -----------------------------------------------------------------------
    /** @name Public constants */
    //@{
    /**
     * The severity of the error described by the <code>DOMError</code>.
     *
     * @since DOM Level 3
     */
    enum ErrorSeverity
    {
        DOM_SEVERITY_WARNING     = 0,
        DOM_SEVERITY_ERROR       = 1,
        DOM_SEVERITY_FATAL_ERROR = 2
    };
    //@}


    // -----------------------------------------------------------------------
    //  Virtual DOMError interface
    // -----------------------------------------------------------------------
    /** @name Functions introduced in DOM Level 3 */
    //@{
    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    /**
     * Get the severity of the error
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @see   setSeverity
     * @since DOM Level 3
     */
    virtual short getSeverity() const = 0;

    /**
     * Get the message describing the error that occured.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @see   setMessage
     * @since DOM Level 3
     */
    virtual const XMLCh* getMessage() const = 0;

    /**
     * Get the location of the error
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @see   setLocation
     * @since DOM Level 3
     */
    virtual DOMLocator* getLocation() const = 0;

    /**
     * The related platform dependent exception if any.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @see   setRelatedException
     * @since DOM Level 3
     */
    virtual void* getRelatedException() const = 0;

    /**
     * A <code>XMLCh*</code> indicating which related data is expected in 
     * relatedData. Users should refer to the specification of the error
     * in order to find its <code>XMLCh*</code> type and relatedData
     * definitions if any.
     *
     * Note: As an example, [DOM Level 3 Load and Save] does not keep the
     * [baseURI] property defined on a Processing Instruction information item.
     * Therefore, the DOMBuilder generates a SEVERITY_WARNING with type 
     * "infoset-baseURI" and the lost [baseURI] property represented as a
     * DOMString in the relatedData attribute.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @see   setType
     * @since DOM Level 3
     */
    virtual const XMLCh* getType() const = 0;

    /**
     * The related DOMError.type dependent data if any.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @see   setRelatedData
     * @since DOM Level 3
     */
    virtual void* getRelatedData() const = 0;


    // -----------------------------------------------------------------------
    //  Setter methods
    // -----------------------------------------------------------------------
    /**
     * Set the severity of the error
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @param severity the type of the error to set
     * @see   getLocation
     * @since DOM Level 3
     */
    virtual void setSeverity(const short severity) = 0;

    /**
     * Set the error message
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @param message the error message to set.
     * @see   getMessage
     * @since DOM Level 3
     */
    virtual void setMessage(const XMLCh* const message) = 0;

    /**
     * Set the location of the error
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @param location the location of the error to set.
     * @see   getLocation
     * @since DOM Level 3
     */
    virtual void setLocation(DOMLocator* const location) = 0;

    /**
     * The related platform dependent exception if any.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @param exc the related exception to set.
     * @see   getRelatedException
     * @since DOM Level 3
     */
    virtual void setRelatedException(void* exc) const = 0;

    /**
     * A <code>XMLCh*</code> indicating which related data is expected in 
     * relatedData. Users should refer to the specification of the error
     * in order to find its <code>XMLCh*</code> type and relatedData
     * definitions if any.
     *
     * Note: As an example, [DOM Level 3 Load and Save] does not keep the
     * [baseURI] property defined on a Processing Instruction information item.
     * Therefore, the DOMBuilder generates a SEVERITY_WARNING with type 
     * "infoset-baseURI" and the lost [baseURI] property represented as a
     * DOMString in the relatedData attribute.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @see   getType
     * @since DOM Level 3
     */
    virtual void setType(const XMLCh* type) = 0;

    /**
     * The related DOMError.type dependent data if any.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @see   getRelatedData
     * @since DOM Level 3
     */
    virtual void setRelatedData(void* relatedData) = 0;

    //@}

};

XERCES_CPP_NAMESPACE_END

#endif
