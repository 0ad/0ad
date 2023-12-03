/* Copyright (C) 2021 Wildfire Games.
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

#ifndef INCLUDED_HASHING
#define INCLUDED_HASHING

#include "ps/CStr.h"

/**
 * Hash a string in a cryptographically secure manner.
 * This method is intended to be 'somewhat fast' for password hashing,
 * and should neither be used where a fast real-time hash is wanted,
 * nor for more sensitive passwords.
 * @return a hex-encoded string.
 */
CStr8 HashCryptographically(const CStr8& password, const CStr8& salt);

#endif
