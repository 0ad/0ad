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

#ifndef INCLUDED_COLLADA_DLL
#define INCLUDED_COLLADA_DLL

#ifdef _WIN32
# ifdef COLLADA_DLL
#  define EXPORT extern "C" __declspec(dllexport)
# else
#  define EXPORT extern "C" __declspec(dllimport)
# endif
#elif defined(__GNUC__)
# define EXPORT extern "C" __attribute__ ((visibility ("default")))
#else
# define EXPORT extern "C"
#endif

#define LOG_INFO 0
#define LOG_WARNING 1
#define LOG_ERROR 2

typedef void (*LogFn) (void* cb_data, int severity, const char* text);
typedef void (*OutputFn) (void* cb_data, const char* data, unsigned int length);

/* This version number should be bumped whenever incompatible changes
 * are made, to invalidate old caches. */
#define COLLADA_CONVERTER_VERSION 3

EXPORT void set_logger(LogFn logger, void* cb_data);
EXPORT int set_skeleton_definitions(const char* xml, int length);
EXPORT int convert_dae_to_pmd(const char* dae, OutputFn pmd_writer, void* cb_data);
EXPORT int convert_dae_to_psa(const char* dae, OutputFn psa_writer, void* cb_data);

#endif /* INCLUDED_COLLADA_DLL */
