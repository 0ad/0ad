/* Copyright (c) 2015 Wildfire Games
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

#ifndef INCLUDED_PCH_BOOST
#define INCLUDED_PCH_BOOST

#include "lib/external_libraries/suppress_boost_warnings.h"

#if MSC_VERSION
# define BOOST_HAS_STDINT_H
#endif

// Boost
// .. if this package isn't going to be statically linked, we're better off
// using Boost via DLL. (otherwise, we would have to ensure the exact same
// compiler is used, which is a pain because MSC8, MSC9 and ICC 10 are in use)
#ifndef LIB_STATIC_LINK
# define BOOST_ALL_DYN_LINK
#endif

// don't compile get_system_category() etc, since we don't use them and they
// sometimes cause problems when linking.
// But Filesystem <= 1.43 requires boost::system::posix, so only disable if newer
#include <boost/version.hpp>
#if BOOST_VERSION >= 104400
# define BOOST_SYSTEM_NO_DEPRECATED
#endif

// the following boost libraries have been included in TR1 and are
// thus deemed usable:
#if BOOST_VERSION >= 104400
// Filesystem v3 is included since Boost 1.44
// v2 is deprecated since 1.46 and removed entirely in 1.50
# define BOOST_FILESYSTEM_VERSION 3
#else
# define BOOST_FILESYSTEM_VERSION 2
#endif
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#endif	// #ifndef INCLUDED_PCH_BOOST
