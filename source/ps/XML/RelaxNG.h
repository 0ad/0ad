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

#ifndef INCLUDED_RELAXNG
#define INCLUDED_RELAXNG

#include "lib/file/vfs/vfs.h"
#include "maths/MD5.h"

typedef struct _xmlRelaxNG xmlRelaxNG;
typedef xmlRelaxNG *xmlRelaxNGPtr;
typedef struct _xmlDoc xmlDoc;
typedef xmlDoc *xmlDocPtr;

class IRelaxNGGrammar;

class RelaxNGValidator
{
public:
	RelaxNGValidator();
	~RelaxNGValidator();

	bool LoadGrammar(const std::string& grammar);

	bool LoadGrammarFile(const PIVFS& vfs, const VfsPath& grammarPath);

	MD5 GetGrammarHash() const { return m_Hash; }

	bool Validate(const std::wstring& filename, const std::wstring& document) const;

	bool ValidateEncoded(const std::wstring& filename, const std::string& document) const;

	bool ValidateEncoded(xmlDocPtr doc) const;

	bool CanValidate() const;

private:
	MD5 m_Hash;
	xmlRelaxNGPtr m_Schema;
};

/**
 * There should be no references to validators or schemas outside of the cache anymore when calling this.
 */
void ClearSchemaCache();

#endif // INCLUDED_RELAXNG
