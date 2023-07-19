/* Copyright (C) 2023 Wildfire Games.
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

#ifndef INCLUDED_WVERSION
#define INCLUDED_WVERSION

// (same format as WINVER)
const size_t WVERSION_2K    = 0x0500;
const size_t WVERSION_XP    = 0x0501;
const size_t WVERSION_XP64  = 0x0502;
const size_t WVERSION_VISTA = 0x0600;
const size_t WVERSION_7     = 0x0601;
const size_t WVERSION_8     = 0x0602;
const size_t WVERSION_8_1   = 0x0603;
const size_t WVERSION_10    = 0x0604;

/**
 * @return short textual representation of the version
 **/
const char* wversion_Family();

#endif	// #ifndef INCLUDED_WVERSION
