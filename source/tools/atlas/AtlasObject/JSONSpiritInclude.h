/* Copyright (C) 2015 Wildfire Games.
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

 // We use this special header for including the JSONSpirit header because some tweaking is needed to disable warnings.
#ifndef JSON_SPIRIT_INCLUDE_H
#define JSON_SPIRIT_INCLUDE_H

#ifdef __GNUC__
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#ifdef _MSC_VER
 #pragma warning(disable: 4100)
 #pragma warning(disable: 4512)
#endif

# include "json_spirit_writer_template.h"
# include "json_spirit_reader_template.h"

#ifdef _MSC_VER
 #pragma warning(default: 4100)
 #pragma warning(default: 4512)
#endif

#ifdef __GNUC__
# pragma GCC diagnostic pop
#endif

#endif // JSON_SPIRIT_INCLUDE_H
