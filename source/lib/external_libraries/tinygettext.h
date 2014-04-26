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

/*
 * Bring in the TinyGettext header file.
 */

#ifndef INCLUDED_TINYGETTEXT
#define INCLUDED_TINYGETTEXT

#if MSC_VERSION
# pragma warning(push)
# pragma warning(disable:4251) // "class X needs to have dll-interface to be used by clients of class Y"
# pragma warning(disable:4800) // "forcing value to bool 'true' or 'false' (performance warning)"
#endif

#include <tinygettext/tinygettext.hpp>
#include <tinygettext/po_parser.hpp>
#include <tinygettext/log.hpp>

#if MSC_VERSION
# pragma warning(pop)
#endif

#endif	// INCLUDED_TINYGETTEXT
