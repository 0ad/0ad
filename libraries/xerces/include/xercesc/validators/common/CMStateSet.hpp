/*
 * Copyright 1999-2001,2004 The Apache Software Foundation.
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
 * $Id: CMStateSet.hpp 191054 2005-06-17 02:56:35Z jberry $
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
        XMemory(toCopy)
      , fBitCount(toCopy.fBitCount)
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
