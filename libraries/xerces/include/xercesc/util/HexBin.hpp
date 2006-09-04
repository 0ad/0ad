/*
 * Copyright 2001,2004 The Apache Software Foundation.
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
 * $Id: HexBin.hpp 176236 2004-12-10 10:37:56Z cargilld $
 */

#ifndef HEXBIN_HPP
#define HEXBIN_HPP

#include <xercesc/util/PlatformUtils.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class XMLUTIL_EXPORT HexBin
{
public :
    //@{

    /**
     * return the length of hexData in terms of HexBinary.
     *
     * @param hexData A string containing the HexBinary
     *
     * return: -1 if it contains any invalid HexBinary
     *         the length of the HexNumber otherwise.
     */

    static int  getDataLength(const XMLCh* const hexData);

     /**
     * check an array of data against the Hex table.
     *
     * @param hexData A string containing the HexBinary
     *
     * return: false if it contains any invalid HexBinary
     *         true otherwise.
     */

    static bool isArrayByteHex(const XMLCh* const hexData);

     /**
     * get canonical representation
     *
     * Caller is responsible for the proper deallcation
     * of the string returned.
     * 
     * @param hexData A string containing the HexBinary
     * @param manager The MemoryManager to use to allocate the string
     *
     * return: the canonical representation of the HexBinary
     *         if it is a valid HexBinary, 
     *         0 otherwise
     */

    static XMLCh* getCanonicalRepresentation
                  (
                      const XMLCh*          const hexData
                    ,       MemoryManager*  const manager = XMLPlatformUtils::fgMemoryManager
                  );

    /**
     * Decodes HexBinary data into XMLCh
     *
     * NOTE: The returned buffer is dynamically allocated and is the
     * responsibility of the caller to delete it when not longer needed.
     * You can call XMLString::release to release this returned buffer.
     *
     * If a memory manager is provided, ask the memory manager to de-allocate
     * the returned buffer.
     *
     * @param hexData HexBinary data in XMLCh stream.
     * @param manager client provided memory manager
     * @return Decoded binary data in XMLCh stream,
     *      or NULL if input data can not be decoded.
     * @see   XMLString::release(XMLCh**)
     * @deprecated use decodeToXMLByte instead.
     */

    static XMLCh* decode(
                         const XMLCh*          const    hexData
                       ,       MemoryManager*  const    manager = XMLPlatformUtils::fgMemoryManager
                        );

   /**
     * Decodes HexBinary data into XMLByte
     *
     * NOTE: The returned buffer is dynamically allocated and is the
     * responsibility of the caller to delete it when not longer needed.
     * You can call XMLString::release to release this returned buffer.
     *
     * If a memory manager is provided, ask the memory manager to de-allocate
     * the returned buffer.
     *
     * @param hexData HexBinary data in XMLCh stream.
     * @param manager client provided memory manager
     * @return Decoded binary data in XMLByte stream,
     *      or NULL if input data can not be decoded.
     * @see   XMLString::release(XMLByte**)
     */
    static XMLByte* decodeToXMLByte(
                         const XMLCh*          const    hexData
                       ,       MemoryManager*  const    manager = XMLPlatformUtils::fgMemoryManager
                        );


    //@}

private :

    // -----------------------------------------------------------------------
    //  Helper methods
    // -----------------------------------------------------------------------

    static void init();

    static bool isHex(const XMLCh& octect);

    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    HexBin();
    HexBin(const HexBin&);
    HexBin& operator=(const HexBin&);

    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  isInitialized
    //
    //     set once hexNumberTable is initalized.
    //
    //  hexNumberTable
    //
    //     arrany holding valid hexNumber character.
    //
    // -----------------------------------------------------------------------
    static bool       isInitialized;
    static XMLByte    hexNumberTable[];
};

XERCES_CPP_NAMESPACE_END

#endif
