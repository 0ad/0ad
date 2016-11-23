/* Copyright (c) 2013 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef OSX_STL_FIXES_H
#define OSX_STL_FIXES_H

#include <istream>
#include <ostream>
#include <AvailabilityMacros.h> // MAC_OS_X_VERSION_*

/**
 * This file adds some explicit template instantiations that are
 * declared external on 10.6+ SDKs but are missing from stdc++ on 10.5
 * (this causes a failure to load due to missing symbols on 10.5)
 **/

#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1060 && MAC_OS_X_VERSION_MIN_REQUIRED < 1060

_GLIBCXX_BEGIN_NAMESPACE(std)


// ostream_insert.h:
# if _GLIBCXX_EXTERN_TEMPLATE
	template ostream& __ostream_insert(ostream&, const char*, streamsize);
#  ifdef _GLIBCXX_USE_WCHAR_T
	template wostream& __ostream_insert(wostream&, const wchar_t*,
					streamsize);
#  endif
# endif


// istream.tcc:
# if _GLIBCXX_EXTERN_TEMPLATE
  template istream& istream::_M_extract(unsigned short&);
  template istream& istream::_M_extract(unsigned int&);
  template istream& istream::_M_extract(long&);
  template istream& istream::_M_extract(unsigned long&);
  template istream& istream::_M_extract(bool&);
#  ifdef _GLIBCXX_USE_LONG_LONG
  template istream& istream::_M_extract(long long&);
  template istream& istream::_M_extract(unsigned long long&);
#  endif
  template istream& istream::_M_extract(float&);
  template istream& istream::_M_extract(double&);
  template istream& istream::_M_extract(long double&);
  template istream& istream::_M_extract(void*&);

  template class basic_iostream<char>;

#  ifdef _GLIBCXX_USE_WCHAR_T
  template wistream& wistream::_M_extract(unsigned short&);
  template wistream& wistream::_M_extract(unsigned int&);
  template wistream& wistream::_M_extract(long&);
  template wistream& wistream::_M_extract(unsigned long&);
  template wistream& wistream::_M_extract(bool&);
#   ifdef _GLIBCXX_USE_LONG_LONG
  template wistream& wistream::_M_extract(long long&);
  template wistream& wistream::_M_extract(unsigned long long&);
#   endif
  template wistream& wistream::_M_extract(float&);
  template wistream& wistream::_M_extract(double&);
  template wistream& wistream::_M_extract(long double&);
  template wistream& wistream::_M_extract(void*&);

  template class basic_iostream<wchar_t>;
#  endif
# endif


// ostream.tcc:
# if _GLIBCXX_EXTERN_TEMPLATE
  template ostream& ostream::_M_insert(long);
  template ostream& ostream::_M_insert(unsigned long);
  template ostream& ostream::_M_insert(bool);
#  ifdef _GLIBCXX_USE_LONG_LONG
  template ostream& ostream::_M_insert(long long);
  template ostream& ostream::_M_insert(unsigned long long);
#  endif
  template ostream& ostream::_M_insert(double);
  template ostream& ostream::_M_insert(long double);
  template ostream& ostream::_M_insert(const void*);

#  ifdef _GLIBCXX_USE_WCHAR_T
  template wostream& wostream::_M_insert(long);
  template wostream& wostream::_M_insert(unsigned long);
  template wostream& wostream::_M_insert(bool);
#   ifdef _GLIBCXX_USE_LONG_LONG
  template wostream& wostream::_M_insert(long long);
  template wostream& wostream::_M_insert(unsigned long long);
#   endif
  template wostream& wostream::_M_insert(double);
  template wostream& wostream::_M_insert(long double);
  template wostream& wostream::_M_insert(const void*);
#  endif
# endif


_GLIBCXX_END_NAMESPACE

#endif // MAC_OS_X_VERSION_MAX_ALLOWED >= 1060 && MAC_OS_X_VERSION_MIN_REQUIRED < 1060

#endif // OSX_STL_FIXES_H
