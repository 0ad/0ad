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

#include "precompiled.h"

#include "PreprocessorWrapper.h"

#include "graphics/ShaderDefines.h"
#include "ps/CLogger.h"
#include "ps/Profile.h"

#include <cctype>
#include <string>
#include <string_view>
#include <vector>

namespace
{

struct MatchIncludeResult
{
	bool found;
	bool error;
	size_t nextLineStart;
	size_t pathFirst, pathLast;

	static MatchIncludeResult MakeNotFound(const std::string_view& source, size_t pos)
	{
		while (pos < source.size() && source[pos] != '\n')
			++pos;
		return MatchIncludeResult{
			false, false, pos < source.size() ? pos + 1 : source.size(), 0, 0};
	}

	static MatchIncludeResult MakeError(
		const char* message, const std::string_view& source, const size_t lineStart, const size_t currentPos)
	{
		ENSURE(currentPos >= lineStart);
		size_t lineEnd = currentPos;
		while (lineEnd < source.size() && source[lineEnd] != '\n' && source[lineEnd] != '\r')
			++lineEnd;
		const std::string_view line = source.substr(lineStart, lineEnd - lineStart);
		while (lineEnd < source.size() && source[lineEnd] != '\n')
			++lineEnd;
		const size_t nextLineStart = lineEnd < source.size() ? lineEnd + 1 : source.size();
		LOGERROR("Preprocessor error: %s: '%s'\n", message, std::string(line).c_str());
		return MatchIncludeResult{false, true, nextLineStart, 0, 0};
	}
};

MatchIncludeResult MatchIncludeUntilEOLorEOS(const std::string_view& source, const size_t lineStart)
{
	// We need to match a line like this:
	// ^[ \t]*#[ \t]*include[ \t]*"[^"]+".*$
	//  ^     ^^     ^      ^     ^^    ^
	//  1     23     4      5     67    8    <- steps
	const CStr INCLUDE = "include";
	size_t pos = lineStart;
	// Matching step #1.
	while (pos < source.size() && std::isblank(source[pos]))
		++pos;
	// Matching step #2.
	if (pos == source.size() || source[pos] != '#')
		return MatchIncludeResult::MakeNotFound(source, pos);
	++pos;
	// Matching step #3.
	while (pos < source.size() && std::isblank(source[pos]))
		++pos;
	// Matching step #4.
	if (pos + INCLUDE.size() >= source.size() || source.substr(pos, INCLUDE.size()) != INCLUDE)
		return MatchIncludeResult::MakeNotFound(source, pos);
	pos += INCLUDE.size();
	// Matching step #5.
	while (pos < source.size() && std::isblank(source[pos]))
		++pos;
	// Matching step #6.
	if (pos == source.size() || source[pos] != '"')
		return MatchIncludeResult::MakeError("#include should be followed by quote", source, lineStart, pos);
	++pos;
	// Matching step #7.
	const size_t pathFirst = pos;
	while (pos < source.size() && source[pos] != '"' && source[pos] != '\n')
		++pos;
	const size_t pathLast = pos;
	// Matching step #8.
	if (pos == source.size() || source[pos] != '"')
		return MatchIncludeResult::MakeError("#include has invalid quote pair", source, lineStart, pos);
	if (pathLast - pathFirst <= 1)
		return MatchIncludeResult::MakeError("#include path shouldn't be empty", source, lineStart, pos);
	while (pos < source.size() && source[pos] != '\n')
		++pos;
	return MatchIncludeResult{true, false, pos < source.size() ? pos + 1 : source.size(), pathFirst, pathLast};
}

bool ResolveIncludesImpl(
	std::string_view currentPart,
	std::unordered_map<CStr, CStr>& includeCache, const CPreprocessorWrapper::IncludeRetrieverCallback& includeCallback,
	std::vector<std::string>& chunks, std::vector<std::string_view>& processedParts)
{
	static const CStr lineDirective = "#line ";
	for (size_t lineStart = 0, line = 1; lineStart < currentPart.size(); ++line)
	{
		MatchIncludeResult match = MatchIncludeUntilEOLorEOS(currentPart, lineStart);
		if (match.error)
			return {};
		else if (!match.found)
		{
			if (lineStart + lineDirective.size() < currentPart.size() &&
				currentPart.substr(lineStart, lineDirective.size()) == lineDirective)
			{
				size_t newLineNumber = 0;
				size_t pos = lineStart + lineDirective.size();
				while (pos < match.nextLineStart && std::isdigit(currentPart[pos]))
				{
					newLineNumber = newLineNumber * 10 + (currentPart[pos] - '0');
					++pos;
				}
				if (newLineNumber > 0)
					line = newLineNumber - 1;
			}

			lineStart = match.nextLineStart;
			continue;
		}
		const std::string path(currentPart.substr(match.pathFirst, match.pathLast - match.pathFirst));
		auto it = includeCache.find(path);
		if (it == includeCache.end())
		{
			CStr includeContent;
			if (!includeCallback(path, includeContent))
			{
				LOGERROR("Preprocessor error: line %zu: Can't load #include file: '%s'", line, path.c_str());
				return false;
			}
			it = includeCache.emplace(path, std::move(includeContent)).first;
		}
		// We need to insert #line directives to have correct line numbers in errors.
		chunks.emplace_back(lineDirective + "1\n" + it->second + "\n" + lineDirective + CStr::FromUInt(line + 1) + "\n");
		processedParts.emplace_back(currentPart.substr(0, lineStart));
		ResolveIncludesImpl(chunks.back(), includeCache, includeCallback, chunks, processedParts);
		currentPart = currentPart.substr(match.nextLineStart);
		lineStart = 0;
	}
	if (!currentPart.empty())
		processedParts.emplace_back(currentPart);
	return true;
}

} // anonymous namespace

void CPreprocessorWrapper::PyrogenesisShaderError(int iLine, const char* iError, const Ogre::CPreprocessor::Token* iToken)
{
	if (iToken)
		LOGERROR("Preprocessor error: line %d: %s: '%s'\n", iLine, iError, std::string(iToken->String, iToken->Length).c_str());
	else
		LOGERROR("Preprocessor error: line %d: %s\n", iLine, iError);
}

CPreprocessorWrapper::CPreprocessorWrapper()
	: CPreprocessorWrapper(IncludeRetrieverCallback{})
{
}

CPreprocessorWrapper::CPreprocessorWrapper(const IncludeRetrieverCallback& includeCallback)
	: m_IncludeCallback(includeCallback)
{
	Ogre::CPreprocessor::ErrorHandler = CPreprocessorWrapper::PyrogenesisShaderError;
}

void CPreprocessorWrapper::AddDefine(const char* name, const char* value)
{
	m_Preprocessor.Define(name, strlen(name), value, strlen(value));
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
		LOGERROR("Failed to parse conditional expression '%s'", expr.c_str());
		return false;
	}

	bool ret = (memchr(output, '1', len) != NULL);

	// Free output if it's not inside the source string
	if (!(output >= input.c_str() && output < input.c_str() + input.size()))
		free(output);

	return ret;

}

CStr CPreprocessorWrapper::ResolveIncludes(const CStr& source)
{
	// Stores intermediate blocks of text to avoid additional copying. Should
	// be constructed before views and destroyed after (currently guaranteed
	// by stack).
	std::vector<std::string> chunks;
	// After resolving the following vector should contain a complete list
	// to concatenate.
	std::vector<std::string_view> processedParts;
	ResolveIncludesImpl(source, m_IncludeCache, m_IncludeCallback, chunks, processedParts);
	std::size_t totalSize = 0;
	for (const std::string_view& part : processedParts)
		totalSize += part.size();
	std::string processedSource;
	processedSource.reserve(totalSize);
	for (const std::string_view& part : processedParts)
		processedSource.append(part);
	return processedSource;
}

CStr CPreprocessorWrapper::Preprocess(const CStr& input)
{
	PROFILE("Preprocess shader source");

	CStr source = ResolveIncludes(input);

	size_t len = 0;
	char* output = m_Preprocessor.Parse(source.c_str(), source.size(), len);

	if (!output)
	{
		LOGERROR("Shader preprocessing failed");
		return "";
	}

	CStr ret(output, len);

	// Free output if it's not inside the source string
	if (!(output >= source.c_str() && output < source.c_str() + source.size()))
		free(output);

	return ret;
}
