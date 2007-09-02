/*
 * wxJavaScript - type.h
 *
 * Copyright (c) 2002-2007 Franky Braem and the wxJavaScript project
 *
 * Project Info: http://www.wxjavascript.net or http://wxjs.sourceforge.net
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 *
 * $Id: type.h 600 2007-03-07 22:08:44Z fbraem $
 */
#ifndef _wxJS_Type_H
#define _wxJS_Type_H

#include <js/jsapi.h>
#include <js/jsdate.h>

#include <wx/datetime.h>
#include "strsptr.h"

class wxStringList;
class wxArrayString;

namespace wxjs
{
    template<class T>
    bool FromJS(JSContext *cx, jsval v, T& to);

    template<>
    bool FromJS<int>(JSContext *cx, jsval v, int &to);

    template<>
    bool FromJS<unsigned int>(JSContext *cx, jsval v, unsigned int &to);

    template<>
    bool FromJS<unsigned long>(JSContext *cx, jsval v, unsigned long &to);

    template<>
    bool FromJS<long>(JSContext *cx, jsval v, long &to);

    template<>
    bool FromJS<double>(JSContext *cx, jsval v, double &to);

    template<>
    bool FromJS<long long>(JSContext *cx, jsval v, long long &to);

    template<>
    bool FromJS<bool>(JSContext *cx, jsval v, bool &to);

    template<>
    bool FromJS<wxString>(JSContext *cx, jsval v, wxString &to);

    template<>
    bool FromJS<wxDateTime>(JSContext *cx, jsval v, wxDateTime& to);

    template<>
    bool FromJS<StringsPtr>(JSContext *cx, jsval v, StringsPtr &to);

    template<>
    bool FromJS<wxArrayString>(JSContext *cx, jsval v, wxArrayString &to);

    template<>
    bool FromJS<wxStringList>(JSContext *cx, jsval v, wxStringList &to);

    template<class T>
    jsval ToJS(JSContext *cx, const T &wx);

    template<>
    jsval ToJS<int>(JSContext *cx, const int &from);

    template<>
    jsval ToJS<unsigned int>(JSContext *cx, const unsigned int &from);

    template<>
    jsval ToJS<long>(JSContext *cx, const long &from);

    template<>
    jsval ToJS<unsigned long>(JSContext *cx, const unsigned long&from);

    template<>
    jsval ToJS<float>(JSContext *cx, const float& from);

    template<>
    jsval ToJS<double>(JSContext *cx, const double &from);

    template<>
    jsval ToJS<bool>(JSContext *cx, const bool &from);

    template<>
    jsval ToJS<wxString>(JSContext *cx, const wxString &from);

    template<>
    jsval ToJS<wxDateTime>(JSContext *cx, const wxDateTime &from);

    template<>
    jsval ToJS<wxArrayString>(JSContext *cx, const wxArrayString &from);

    template<>
    jsval ToJS<wxStringList>(JSContext *cx, const wxStringList &from);

}; // Namespace wxjs

#endif // _wxJS_Type_H
