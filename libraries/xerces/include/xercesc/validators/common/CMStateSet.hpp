/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 1999-2001 The Apache Software Foundation.  All rights
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
 * $Log: CMStateSet.hpp,v $
 * Revision 1.6  2003/12/17 00:18:38  cargilld
 * Update to memory management so that the static memory manager (one used to call Initialize) is only for static data.
 *
 * Revision 1.5  2003/05/16 21:43:20  knoaman
 * Memory manager implementation: Modify constructors to pass in the memory manager.
 *
 * Revision 1.4  2003/05/15 18:48:27  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.3  2002/11/04 14:54:58  tng
 * C++ Namespace Support.
 *
 * Revision 1.2  2002/07/16 12:50:08  tng
 * [Bug 10651] CMStateSet.hpp includes both memory.h and string.h.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:38  peiyongz
 * sane_include
 *
 * Revision 1.5  2001/08/16 21:51:43  peiyongz
 * hashCode() added
 *
 * Revision 1.4  2001/05/11 13:27:17  tng
 * Copyright update.
 *
 * Revision 1.3  2001/05/03 21:02:28  tng
 * Schema: Add SubstitutionGroupComparator and update exception messages.  By Pei Yong Zhang.
 *
 * Revision 1.2  2001/02/27 14:48:46  tng
 * Schema: Add CMAny and ContentLeafNameTypeVector, by Pei Yong Zhang
 *
 * Revision 1.1  2001/02/16 14:17:29  tng
 * Schema: Move the common Content Model files that are shared by DTD
 * and schema from 'DTD' folder to 'common' folder.  By Pei Yong Zhang.
 *
 * Revision 1.4  2000/03/02 19:55:37  roddey
 * This checkin includes many changes done while waiting for the
 * 1.1.0 code to be finished. I can't list them all here, but a list is
 * available elsewhere.
 *
 * Revision 1.3  2000/02/24 20:16:48  abagchi
 * Swat for removing Log from API docs
 *
 * Revision 1.2  2000/02/09 21:42:36  abagchi
 * Copyright swat
 *
 * Revision 1.1.1.1  1999/11/09 01:03:06  twl
 * Initial checkin
 *
 * Revision 1.3  1999/11/08 20:45:36  rahul
 * Swat for adding in Product name and CVS comment log variable.
 *
 */

//  DESCRIPTION:
//
//  This class is a specialized bitset class for the content model code of
//  the validator. It assumes that its never called with two objects of
//  different bit counts, and that bit sets smaller than 64 bits are far
//  and away the most common. So it can be a lot more optimized than a general
//  purpose utility bitset class
//

#if !defined(CMSTATESET_HPP)
#define CMSTATESET_HPP

#include <xercesc/util/ArrayIndexOutOfBoundsException.hpp>
#include <xercesc/util/RuntimeException.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/framework/MemoryManager.hpp>
#include <string.h>

XERCES_CPP_NAMESPACE_BEGIN

class CMStateSet : public XMemory
{
public :
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    CMStateSet( const unsigned int bitCount
              , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager) :

        fBitCount(bitCount)
        , fByteArray(0)
        , fMemoryManager(manager)
    {
        //
        //  See if we need to allocate the byte array or whether we can live
        //  within the 64 bit high performance scheme.
        //
        if (fBitCount > 64)
        {
            fByteCount = fBitCount / 8;
            if (fBitCount % 8)
                fByteCount++;
            fByteArray = (XMLByte*) fMemoryManager->allocate(fByteCount*sizeof(XMLByte)); //new XMLByte[fByteCount];
        }

        // Init all the bits to zero
        zeroBits();
    }


    /*
     * This method with the 'for' statement (commented out) cannot be made inline
     * because the antiquated CC (CFront) compiler under HPUX 10.20 does not allow
     * the 'for' statement inside any inline method. Unfortunately,
     * we have to support it. So instead, we use memcpy().
     */

    CMStateSet(const CMStateSet& toCopy) :
        fBitCount(toCopy.fBitCount)
      , fByteArray(0)
      , fMemoryManager(toCopy.fMemoryManager)
    {
        //
        //  See if we need to allocate the byte array or whether we can live
        //  within the 64 bit high performance scheme.
        //
        if (fBitCount > 64)
        {
            fByteCount = fBitCount / 8;
            if (fBitCount % 8)
                fByteCount++;
            fByteArray = (XMLByte*) fMemoryManager->allocate(fByteCount*sizeof(XMLByte)); //new XMLByte[fByteCount];

            memcpy((void *) fByteArray,
                   (const void *) toCopy.fByteArray,
                   fByteCount * sizeof(XMLByte));

            // for (unsigned int index = 0; index < fByteCount; index++)
            //     fByteArray[index] = toCopy.fByteArray[index];
        }
         else
        {
            fBits1 = toCopy.fBits1;
            fBits2 = toCopy.fBits2;
        }
    }

    ~CMStateSet()
    {
        if (fByteArray)
            fMemoryManager->deallocate(fByteArray); //delete [] fByteArray;
    }


    // -----------------------------------------------------------------------
    //  Set manipulation methods
    // -----------------------------------------------------------------------
    void operator&=(const CMStateSet& setToAnd)
    {
        if (fBitCount < 65)
        {
            fBits1 &= setToAnd.fBits1;
            fBits2 &= setToAnd.fBits2;
        }
         else
        {
            for (unsigned int index = 0; index < fByteCount; index++)
                fByteArray[index] &= setToAnd.fByteArray[index];
        }
    }

    void operator|=(const CMStateSet& setToOr)
    {
        if (fBitCount < 65)
        {
            fBits1 |= setToOr.fBits1;
            fBits2 |= setToOr.fBits2;
        }
         else
        {
            for (unsigned int index = 0; index < fByteCount; index++)
                fByteArray[index] |= setToOr.fByteArray[index];
        }
    }

    bool operator==(const CMStateSet& setToCompare) const
    {
        if (fBitCount != setToCompare.fBitCount)
            return false;

        if (fBitCount < 65)
        {
            return ((fBits1 == setToCompare.fBits1)
            &&      (fBits2 == setToCompare.fBits2));
        }

        for (unsigned int index = 0; index < fByteCount; index++)
        {
            if (fByteArray[index] != setToCompare.fByteArray[index])
                return false;
        }
        return true;
    }

    CMStateSet& operator=(const CMStateSet& srcSet)
    {
        if (this == &srcSet)
            return *this;

        // They have to be the same size
        if (fBitCount != srcSet.fBitCount)
            ThrowXMLwithMemMgr(RuntimeException, XMLExcepts::Bitset_NotEqualSize, fMemoryManager);

        if (fBitCount < 65)
        {
            fBits1 = srcSet.fBits1;
            fBits2 = srcSet.fBits2;
        }
         else
        {
            for (unsigned int index = 0; index < fByteCount; index++)
                fByteArray[index] = srcSet.fByteArray[index];
        }
        return *this;
    }


    bool getBit(const unsigned int bitToGet) const
    {
        if (bitToGet >= fBitCount)
            ThrowXMLwithMemMgr(ArrayIndexOutOfBoundsException, XMLExcepts::Bitset_BadIndex, fMemoryManager);

        if (fBitCount < 65)
        {
            unsigned int mask = (0x1UL << (bitToGet % 32));
            if (bitToGet < 32)
                return ((fBits1 & mask) != 0);
            else
                return ((fBits2 & mask) != 0);
        }

        // Create the mask and byte values
        const XMLByte mask1 = XMLByte(0x1 << (bitToGet % 8));
        const unsigned int byteOfs = bitToGet >> 3;

        // And access the right bit and byte
        return ((fByteArray[byteOfs] & mask1) != 0);
    }

    bool isEmpty() const
    {
        if (fBitCount < 65)
            return ((fBits1 == 0) && (fBits2 == 0));

        for (unsigned int index = 0; index < fByteCount; index++)
        {
            if (fByteArray[index] != 0)
                return false;
        }
        return true;
    }

    void setBit(const unsigned int bitToSet)
    {
        if (bitToSet >= fBitCount)
            ThrowXMLwithMemMgr(ArrayIndexOutOfBoundsException, XMLExcepts::Bitset_BadIndex, fMemoryManager);

        if (fBitCount < 65)
        {
            const unsigned int mask = (0x1UL << (bitToSet % 32));
            if (bitToSet < 32)
            {
                fBits1 &= ~mask;
                fBits1 |= mask;
            }
             else
            {
                fBits2 &= ~mask;
                fBits2 |= mask;
            }
        }
         else
        {
            // Create the mask and byte values
            const XMLByte mask1 = XMLByte(0x1 << (bitToSet % 8));
            const unsigned int byteOfs = bitToSet >> 3;

            // And access the right bit and byte
            fByteArray[byteOfs] &= ~mask1;
            fByteArray[byteOfs] |= mask1;
        }
    }

    void zeroBits()
    {
        if (fBitCount < 65)
        {
            fBits1 = 0;
            fBits2 = 0;
        }
         else
        {
            for (unsigned int index = 0; index < fByteCount; index++)
                fByteArray[index] = 0;
        }
    }

    int hashCode() const
    {
        if (fBitCount < 65)
        {
            return fBits1+ fBits2 * 31;
        }
        else
        {
            int hash = 0;
            for (int index = fByteCount - 1; index >= 0; index--)
                hash = fByteArray[index] + hash * 31;
            return hash;
        }

    }

private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    CMStateSet();


    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fBitCount
    //      The count of bits that the outside world wants to support,
    //      so its the max bit index plus one.
    //
    //  fByteCount
    //      If the bit count is > 64, then we use the fByteArray member to
    //      store the bits, and this indicates its size in bytes. Otherwise
    //      its value is meaningless and unset.
    //
    //  fBits1
    //  fBits2
    //      When the bit count is <= 64 (very common), these hold the bits.
    //      Otherwise, the fByteArray member holds htem.
    //
    //  fByteArray
    //      The array of bytes used when the bit count is > 64. It is
    //      allocated as required.
    // -----------------------------------------------------------------------
    unsigned int    fBitCount;
    unsigned int    fByteCount;
    unsigned int    fBits1;
    unsigned int    fBits2;
    XMLByte*        fByteArray;
    MemoryManager*  fMemoryManager;
};

XERCES_CPP_NAMESPACE_END

#endif
