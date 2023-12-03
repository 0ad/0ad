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

#ifndef INCLUDED_PREPROCESSORWRAPPER
#define INCLUDED_PREPROCESSORWRAPPER

#include "ps/CStr.h"
#include "third_party/ogre3d_preprocessor/OgreGLSLPreprocessor.h"

#include <functional>
#include <unordered_map>

class CShaderDefines;

/**
 * Convenience wrapper around CPreprocessor.
 */
class CPreprocessorWrapper
{
public:
	using IncludeRetrieverCallback = std::function<bool(const CStr&, CStr& out)>;

	CPreprocessorWrapper();
	CPreprocessorWrapper(const IncludeRetrieverCallback& includeCallback);

	void AddDefine(const char* name, const char* value);

	void AddDefines(const CShaderDefines& defines);

	bool TestConditional(const CStr& expr);

	// Find all #include directives in the input and replace them by
	// by a file content from the directive's argument. Parsing is strict
	// and simple. The directive will be expanded in comments and multiline
	// strings.
	CStr ResolveIncludes(const CStr& source);

	CStr Preprocess(const CStr& input);

	static void PyrogenesisShaderError(int iLine, const char* iError, const Ogre::CPreprocessor::Token* iToken);

private:
	Ogre::CPreprocessor m_Preprocessor;
	IncludeRetrieverCallback m_IncludeCallback;
	std::unordered_map<CStr, CStr> m_IncludeCache;
};

#endif // INCLUDED_PREPROCESSORWRAPPER
