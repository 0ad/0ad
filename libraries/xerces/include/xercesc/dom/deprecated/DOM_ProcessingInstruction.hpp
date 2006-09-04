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
 * $Id: DOM_ProcessingInstruction.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

#ifndef DOM_ProcessingInstruction_HEADER_GUARD_
#define DOM_ProcessingInstruction_HEADER_GUARD_

#include <xercesc/util/XercesDefs.hpp>
#include "DOM_Node.hpp"

XERCES_CPP_NAMESPACE_BEGIN


class ProcessingInstructionImpl;

/**
 * The <code>ProcessingInstruction</code> interface represents a  "processing
 * instruction", used in XML as a way to keep processor-specific information
 * in the text of the document.
 */
class  DEPRECATED_DOM_EXPORT DOM_ProcessingInstruction: public DOM_Node {
public:
    /** @name Constructors and assignment operator */
    //@{
    /**
      * Default constructor for DOM_ProcessingInstruction.  The resulting object
      *  does not refer to an actual PI node; it will compare == to 0, and is similar
      * to a null object reference variable in Java.  It may subsequently be
      * assigned to refer to an actual PI node.
      * <p>
      * New Processing Instruction nodes are created by DOM_Document::createProcessingInstruction().
      *
      *
      */
    DOM_ProcessingInstruction();

    /**
      * Copy constructor.  Creates a new <code>DOM_ProcessingInstruction</code> that refers to the
      * same underlying node as the original.  See also DOM_Node::clone(),
      * which will copy the actual PI node, rather than just creating a new
      * reference to the original node.
      *
      * @param other The object to be copied.
      */
    DOM_ProcessingInstruction(const DOM_ProcessingInstruction &other);

    /**
      * Assignment operator.
      *
      * @param other The object to be copied.
      */
    DOM_ProcessingInstruction & operator = (const DOM_ProcessingInstruction &other);

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
    DOM_ProcessingInstruction & operator = (const DOM_NullPtr *val);

    //@}
    /** @name Destructor. */
    //@{
	 /**
	  * Destructor for DOM_processingInstruction.  The object being destroyed is the reference
      * object, not the underlying PI node itself.
	  *
	  */
    ~DOM_ProcessingInstruction();

    //@}
    /** @name Get functions. */
    //@{
    /**
     * The target of this processing instruction.
     *
     * XML defines this as being the
     * first token following the markup that begins the processing instruction.
     */
    DOMString        getTarget() const;

    /**
     * The content of this processing instruction.
     *
     * This is from the first non
     * white space character after the target to the character immediately
     * preceding the <code>?&gt;</code>.
     * @exception DOMException
     *   NO_MODIFICATION_ALLOWED_ERR: Raised when the node is readonly.
     */
    DOMString        getData() const;

    //@}
    /** @name Set functions. */
    //@{
    /**
    * Sets the content of this processing instruction.
    *
    * This is from the first non
    * white space character after the target to the character immediately
    * preceding the <code>?&gt;</code>.
    * @param data The string containing the processing instruction
    */
    void             setData(const DOMString &data);
    //@}

protected:
    DOM_ProcessingInstruction(ProcessingInstructionImpl *impl);

    friend class DOM_Document;

};

XERCES_CPP_NAMESPACE_END

#endif

