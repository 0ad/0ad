/* Copyright (C) 2013 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

// Precompiled headers:

#ifndef INCLUDED_STDAFX
#define INCLUDED_STDAFX

#ifdef USING_PCH
# define HAVE_PCH 1
#else
# define HAVE_PCH 0
#endif

#if defined(_MSC_VER)
# pragma warning(disable:4996)	// deprecated CRT
#endif

#if HAVE_PCH

#define WX_PRECOMP

// Exclude rarely-used stuff from Windows headers
#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN
#endif

#if defined(__GNUC__) && (__GNUC__*100 + __GNUC_MINOR__) >= 402 // (older GCCs don't support this pragma)
# pragma GCC diagnostic ignored "-Wredundant-decls" // triggered by wx/geometry.h
#endif

// Include useful wx headers
#include "wx/wxprec.h"

#include "wx/artprov.h"
#include "wx/cmdproc.h"
#include "wx/colordlg.h"
#include "wx/config.h"
#include "wx/dialog.h"
#include "wx/dir.h"
#include "wx/dnd.h"
#include "wx/docview.h"
#include "wx/file.h"
#include "wx/filename.h"
#include "wx/filesys.h"
#include "wx/glcanvas.h"
#include "wx/image.h"
#include "wx/listctrl.h"
#include "wx/mstream.h"
#include "wx/notebook.h"
#include "wx/progdlg.h"
#include "wx/regex.h"
#include "wx/sound.h"
#include "wx/spinctrl.h"
#include "wx/splitter.h"
#include "wx/tooltip.h"
#include "wx/treectrl.h"
#include "wx/wfstream.h"
#include "wx/zstream.h"

#if defined(__GNUC__) && (__GNUC__*100 + __GNUC_MINOR__) >= 402
# pragma GCC diagnostic warning "-Wredundant-decls" // re-enable
#endif

#include <vector>
#include <string>
#include <set>
#include <stack>
#include <map>
#include <limits>
#include <cassert>

#include <boost/bind.hpp>
#include <boost/version.hpp>
#if BOOST_VERSION >= 104000
# include <boost/signals2.hpp>
#else
# error Atlas requires Boost 1.40 or later
#endif

// Nicer memory-leak detection:
#ifdef _WIN32
# ifdef _DEBUG
#  include <crtdbg.h>
#  define new new(_NORMAL_BLOCK ,__FILE__, __LINE__)
# endif
#endif

#else // HAVE_PCH

// If no PCH, just include the most common headers anyway
# include "wx/wx.h"

#endif // HAVE_PCH

#ifdef _WIN32
# define ATLASDLLIMPEXP extern "C" __declspec(dllexport)
#else
# if __GNUC__ >= 4
#  define ATLASDLLIMPEXP extern "C" __attribute__ ((visibility ("default")))
# else
#  define ATLASDLLIMPEXP extern "C"
# endif
#endif

// Abort with an obvious message if wx isn't Unicode, instead of complaining
// mysteriously when it first discovers wxChar != wchar_t
#ifndef UNICODE
# error This needs to be compiled with a Unicode version of wxWidgets.
#endif

#endif // INCLUDED_STDAFX
