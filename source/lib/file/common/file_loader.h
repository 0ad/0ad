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

#ifndef INCLUDED_FILE_LOADER
#define INCLUDED_FILE_LOADER

struct IFileLoader
{
	virtual ~IFileLoader();

	virtual size_t Precedence() const = 0;
	virtual char LocationCode() const = 0;

	virtual LibError Load(const std::string& name, const shared_ptr<u8>& buf, size_t size) const = 0;
};

typedef shared_ptr<IFileLoader> PIFileLoader;

#endif	// #ifndef INCLUDED_FILE_LOADER
