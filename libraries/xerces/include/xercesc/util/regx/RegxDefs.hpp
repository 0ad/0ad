/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2001-2002 The Apache Software Foundation.  All rights
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
 * originally based on software copyright (c) 2001, International
 * Business Machines, Inc., http://www.ibm.com .  For more information
 * on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

/*
 * $Id: RegxDefs.hpp,v 1.3 2002/11/04 15:17:00 tng Exp $
 */

#if !defined(REGXDEFS_HPP)
#define REGXDEFS_HPP

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/XercesDefs.hpp>
#include <xercesc/util/XMLUniDefs.hpp>

XERCES_CPP_NAMESPACE_BEGIN

const XMLCh fgXMLCategory[] =
{
    chLatin_X, chLatin_M, chLatin_L, chNull
};

const XMLCh fgASCIICategory[] =
{
    chLatin_A, chLatin_S, chLatin_C, chLatin_I, chLatin_I, chNull
};

const XMLCh fgUnicodeCategory[] =
{
    chLatin_U, chLatin_N, chLatin_I, chLatin_C, chLatin_O, chLatin_D,
    chLatin_E, chNull
};

const XMLCh fgBlockCategory[] =
{
    chLatin_B, chLatin_L, chLatin_O, chLatin_C, chLatin_K, chNull
};

const XMLCh fgXMLSpace[] =
{
    chLatin_x, chLatin_m, chLatin_l, chColon, chLatin_i, chLatin_s, chLatin_S,
    chLatin_p, chLatin_a, chLatin_c, chLatin_e, chNull
};

const XMLCh fgXMLDigit[] =
{
    chLatin_x, chLatin_m, chLatin_l, chColon, chLatin_i, chLatin_s, chLatin_D,
    chLatin_i, chLatin_g, chLatin_i, chLatin_t, chNull
};

const XMLCh fgXMLWord[] =
{
    chLatin_x, chLatin_m, chLatin_l, chColon, chLatin_i, chLatin_s, chLatin_W,
    chLatin_o, chLatin_r, chLatin_d, chNull
};

const XMLCh fgXMLNameChar[] =
{
    chLatin_x, chLatin_m, chLatin_l, chColon, chLatin_i, chLatin_s, chLatin_N,
    chLatin_a, chLatin_m, chLatin_e, chLatin_C, chLatin_h, chLatin_a,
	chLatin_r, chNull
};

const XMLCh fgXMLInitialNameChar[] =
{
    chLatin_x, chLatin_m, chLatin_l, chColon, chLatin_i, chLatin_s, chLatin_I,
    chLatin_n, chLatin_i, chLatin_t, chLatin_i, chLatin_a, chLatin_l,
    chLatin_N, chLatin_a, chLatin_m, chLatin_e, chLatin_C, chLatin_h,
    chLatin_a, chLatin_r, chNull
};

const XMLCh fgASCII[] =
{
    chLatin_a, chLatin_s, chLatin_c, chLatin_i, chLatin_i, chColon, chLatin_i,
    chLatin_s, chLatin_A, chLatin_s, chLatin_c, chLatin_i, chLatin_i, chNull
};

const XMLCh fgASCIIDigit[] =
{
    chLatin_a, chLatin_s, chLatin_c, chLatin_i, chLatin_i, chColon, chLatin_i,
    chLatin_s, chLatin_D, chLatin_i, chLatin_g, chLatin_i, chLatin_t, chNull
};

const XMLCh fgASCIIWord[] =
{
    chLatin_a, chLatin_s, chLatin_c, chLatin_i, chLatin_i, chColon, chLatin_i,
    chLatin_s, chLatin_W, chLatin_o, chLatin_r, chLatin_d, chNull
};

const XMLCh fgASCIISpace[] =
{
    chLatin_a, chLatin_s, chLatin_c, chLatin_i, chLatin_i, chColon, chLatin_i,
    chLatin_s, chLatin_S, chLatin_p, chLatin_a, chLatin_c, chLatin_e, chNull
};

const XMLCh fgASCIIXDigit[] =
{
    chLatin_a, chLatin_s, chLatin_c, chLatin_i, chLatin_i, chColon, chLatin_i,
    chLatin_s, chLatin_X, chLatin_D, chLatin_i, chLatin_g, chLatin_i,
    chLatin_t, chNull
};


const XMLCh fgUniAll[] =
{
    chLatin_A, chLatin_L, chLatin_L, chNull
};

const XMLCh fgUniIsAlpha[] =
{
    chLatin_I, chLatin_s, chLatin_A, chLatin_l, chLatin_p, chLatin_h,
    chLatin_a, chNull
};

const XMLCh fgUniIsAlnum[] =
{
    chLatin_I, chLatin_s, chLatin_A, chLatin_l, chLatin_n, chLatin_u,
    chLatin_m, chNull
};

const XMLCh fgUniIsWord[] =
{
    chLatin_I, chLatin_s, chLatin_W, chLatin_o, chLatin_r, chLatin_d,
    chNull
};


const XMLCh fgUniIsDigit[] =
{
    chLatin_I, chLatin_s, chLatin_D, chLatin_i, chLatin_g, chLatin_i,
    chLatin_t, chNull
};

const XMLCh fgUniIsUpper[] =
{
    chLatin_I, chLatin_s, chLatin_U, chLatin_p, chLatin_p, chLatin_e,
    chLatin_r, chNull
};

const XMLCh fgUniIsLower[] =
{
    chLatin_I, chLatin_s, chLatin_L, chLatin_o, chLatin_w, chLatin_e,
    chLatin_r, chNull
};

const XMLCh fgUniIsPunct[] =
{
    chLatin_I, chLatin_s, chLatin_P, chLatin_u, chLatin_n, chLatin_c,
    chLatin_t, chNull
};

const XMLCh fgUniIsSpace[] =
{
	chLatin_I, chLatin_s, chLatin_S, chLatin_p, chLatin_a, chLatin_c,
    chLatin_e, chNull
};

const XMLCh fgUniAssigned[] =
{
    chLatin_A, chLatin_S, chLatin_S, chLatin_I, chLatin_G, chLatin_N,
    chLatin_E, chLatin_D, chNull
};


const XMLCh fgUniDecimalDigit[] =
{
    chLatin_N, chLatin_d, chNull
};

const XMLCh fgBlockIsSpecials[] =
{
    chLatin_I, chLatin_s, chLatin_S, chLatin_p, chLatin_e, chLatin_c, chLatin_i, chLatin_a,
    chLatin_l, chLatin_s, chNull
};

const XMLCh fgBlockIsPrivateUse[] =
{
    chLatin_I, chLatin_s, chLatin_P, chLatin_r, chLatin_i, chLatin_v, chLatin_a, chLatin_t, chLatin_e,
    chLatin_U, chLatin_s, chLatin_e,  chNull
};

const XMLCh fgUniLetter[] =
{
    chLatin_L, chNull
};

const XMLCh fgUniNumber[] =
{
    chLatin_N, chNull
};

const XMLCh fgUniMark[] =
{
    chLatin_M, chNull
};

const XMLCh fgUniSeparator[] =
{
    chLatin_Z, chNull
};

const XMLCh fgUniPunctuation[] =
{
    chLatin_P, chNull
};

const XMLCh fgUniControl[] =
{
    chLatin_C, chNull
};

const XMLCh fgUniSymbol[] =
{
    chLatin_S, chNull
};

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file RegxDefs.hpp
  */

