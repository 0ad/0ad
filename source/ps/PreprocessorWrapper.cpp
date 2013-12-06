/* Copyright (C) 2012 Wildfire Games.
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

#include "precompiled.h"

#include "PreprocessorWrapper.h"

#include "graphics/ShaderDefines.h"
#include "ps/CLogger.h"

void CPreprocessorWrapper::AddDefine(const char* name, const char* value)
{
	m_Preprocessor.Define(name, value);
}

void CPreprocessorWrapper::AddDefines(const CShaderDefines& defines)
{
	std::map<CStrIntern, CStrIntern> map = defines.GetMap();
	for (std::map<CStrIntern, CStrIntern>::const_iterator it = map.begin(); it != map.end(); ++it)
		m_Preprocessor.Define(it->first.c_str(), it->first.length(), it->second.c_str(), it->second.length());
}

bool CPreprocessorWrapper::TestConditional(const CStr& expr)
{
	// Construct a dummy program so we can trigger the preprocessor's expression
	// code without modifying its public API.
	// Be careful that the API buggily returns a statically allocated pointer
	// (which we will try to free()) if the input just causes it to append a single
	// sequence of newlines to the output; the "\n" after the "#endif" is enough
	// to avoid this case.
	CStr input = "#if ";
	input += expr;
	input += "\n1\n#endif\n";

	size_t len = 0;
	char* output = m_Preprocessor.Parse(input.c_str(), input.size(), len);

	if (!output)
	{
		LOGERROR(L"Failed to parse conditional expression '%hs'", expr.c_str());
		return false;
	}

	bool ret = (memchr(output, '1', len) != NULL);

	// Free output if it's not inside the source string
	if (!(output >= input.c_str() && output < input.c_str() + input.size()))
		free(output);

	return ret;

}

CStr CPreprocessorWrapper::Preprocess(const CStr& input)
{
	size_t len = 0;
	char* output = m_Preprocessor.Parse(input.c_str(), input.size(), len);

	if (!output)
	{
		LOGERROR(L"Shader preprocessing failed");
		return "";
	}

	CStr ret(output, len);

	// Free output if it's not inside the source string
	if (!(output >= input.c_str() && output < input.c_str() + input.size()))
		free(output);

	return ret;
}
