/* Copyright (C) 2020 Wildfire Games.
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

class CShaderDefines;

/**
 * Convenience wrapper around CPreprocessor.
 */
class CPreprocessorWrapper
{
public:
	CPreprocessorWrapper();

	void AddDefine(const char* name, const char* value);

	void AddDefines(const CShaderDefines& defines);

	bool TestConditional(const CStr& expr);

	CStr Preprocess(const CStr& input);

	static void PyrogenesisShaderError(int iLine, const char* iError, const Ogre::CPreprocessor::Token* iToken);

private:
	Ogre::CPreprocessor m_Preprocessor;
};

#endif // INCLUDED_PREPROCESSORWRAPPER
