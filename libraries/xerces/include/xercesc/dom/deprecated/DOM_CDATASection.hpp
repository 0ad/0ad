/*
 * Copyright 1999-2002,2004 The Apache Software Foundation.
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
 * $Id: DOM_CDATASection.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

#ifndef DOM_CDataSection_HEADER_GUARD_
#define DOM_CDataSection_HEADER_GUARD_

#include <xercesc/util/XercesDefs.hpp>
#include "DOM_Text.hpp"

XERCES_CPP_NAMESPACE_BEGIN


class CDATASectionImpl;

/**
 * <code>DOM_CDataSection</code> objects refer to the data from an
 * XML CDATA section.  These are used to escape blocks of text containing  characters
 * that would otherwise be regarded as markup.
 *
 * <p>Note that the string data associated with the CDATA section may
 * contain characters that need to be escaped when appearing in an
 * XML document outside of a CDATA section.
 * <p> The <code>DOM_CDATASection</code> class inherits from the
 * <code>DOM_CharacterData</code> class through the <code>Text</code>
 * interface. Adjacent CDATASection nodes are not merged by use
 * of the Element.normalize() method.
 */
class DEPRECATED_DOM_EXPORT DOM_CDATASection: public DOM_Text {
public:
  /** @name Constructors and assignment operators */
  //@{
  /**
    * Default constructor for DOM_CDATASection.  The resulting object does not
    * refer to any actual CData section; it will compare == to 0, and is similar
    * to a null object reference variable in Java.
    *
    */
        DOM_CDATASection();
  /**
    * Copy constructor.  Creates a new <code>DOM_CDataSection</code> that refers to the
    *   same underlying data as the original.  See also <code>DOM_Node::clone()</code>,
    * which will copy the underlying data, rather than just creating a new
    * reference to the original object.
    *
    * @param other The source <code>DOM_CDATASection</code> object
    */
        DOM_CDATASection(const DOM_CDATASection &other);

  /**
    * Assignment operator.
    *
    * @param other The object to be copied.
    */
        DOM_CDATASection & operator = (const DOM_CDATASection &other);

    /**
      * Assignment operator.  This overloaded variant is provided for
      *   the sole purpose of setting a DOM_Node reference variable to
      *   zero.  Nulling out a reference variable in this way will decrement
      *   the reference count on the underlying Node object that the variable
      *   formerly referenced.  This effect is normally obtained when reference
      *   variable goes out of scope, but zeroing them can be useful for
      *   global instances, or for local instances that will remain in scope
      *   for an extended time,  when the storage belonging to the underlying
      *   node needs to be reclaimed.
      *
      * @param val   Only a value of 0, or null, is allowed.
      */
    DOM_CDATASection & operator = (const DOM_NullPtr *val);


    //@}
    /** @name Destructor. */
    //@{
	 /**
	  * Destructor for DOM_CDATASection.
	  *
	  */

	    ~DOM_CDATASection();
    //@}


protected:
	DOM_CDATASection(CDATASectionImpl *);

    friend class DOM_Document;

};

XERCES_CPP_NAMESPACE_END

#endif


