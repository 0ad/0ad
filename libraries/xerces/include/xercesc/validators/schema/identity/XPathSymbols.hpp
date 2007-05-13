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
 * $Id: XPathSymbols.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

#if !defined(XPATHSYMBOLS_HPP)
#define XPATHSYMBOLS_HPP

#include <xercesc/util/XercesDefs.hpp>

XERCES_CPP_NAMESPACE_BEGIN

/*
 * Collection of symbols used to parse a Schema Grammar
 */

class VALIDATORS_EXPORT XPathSymbols
{
public :
    // -----------------------------------------------------------------------
    // Constant data
    // -----------------------------------------------------------------------
    static const XMLCh fgSYMBOL_AND[];
    static const XMLCh fgSYMBOL_OR[];
    static const XMLCh fgSYMBOL_MOD[];
    static const XMLCh fgSYMBOL_DIV[];
    static const XMLCh fgSYMBOL_COMMENT[];
    static const XMLCh fgSYMBOL_TEXT[];
    static const XMLCh fgSYMBOL_PI[];
    static const XMLCh fgSYMBOL_NODE[];
    static const XMLCh fgSYMBOL_ANCESTOR[];
    static const XMLCh fgSYMBOL_ANCESTOR_OR_SELF[];
    static const XMLCh fgSYMBOL_ATTRIBUTE[];
    static const XMLCh fgSYMBOL_CHILD[];
    static const XMLCh fgSYMBOL_DESCENDANT[];
    static const XMLCh fgSYMBOL_DESCENDANT_OR_SELF[];
    static const XMLCh fgSYMBOL_FOLLOWING[];
    static const XMLCh fgSYMBOL_FOLLOWING_SIBLING[];
    static const XMLCh fgSYMBOL_NAMESPACE[];
    static const XMLCh fgSYMBOL_PARENT[];
    static const XMLCh fgSYMBOL_PRECEDING[];
    static const XMLCh fgSYMBOL_PRECEDING_SIBLING[];
    static const XMLCh fgSYMBOL_SELF[];

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    XPathSymbols();
};

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file XPathSymbols.hpp
  */

