/* Copyright (C) 2009 Wildfire Games.
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

/**
 * =========================================================================
 * File        : boost_filesystem.h
 * Project     : 0 A.D.
 * Description : bring in Boost filesystem library
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_BOOST_FILESYSTEM
#define INCLUDED_BOOST_FILESYSTEM

// not W4-clean
#if MSC_VERSION
# pragma warning(push, 3)
#endif

#include "boost/filesystem.hpp"
namespace fs = boost::filesystem;

#if MSC_VERSION
# pragma warning(pop)
#endif

#endif	// #ifndef INCLUDED_BOOST_FILESYSTEM
