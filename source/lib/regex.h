/**
 * =========================================================================
 * File        : regex.h
 * Project     : 0 A.D.
 * Description : minimal regex implementation
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_REGEX
#define INCLUDED_REGEX

/**
 * see if string matches pattern.
 *
 * @param s input string
 * @param w pseudo-regex to match against. case-insensitive;
 * may contain '?' and/or '*' wildcards. if NULL, matches everything.
 *
 * @return 1 if they match, otherwise 0.
 *
 * algorithmfrom http://www.codeproject.com/string/wildcmp.asp.
 **/
extern int match_wildcard(const char* s, const char* w);
/// unicode version of match_wildcard.
extern int match_wildcardw(const wchar_t* s, const wchar_t* w);

#endif	// #ifndef INCLUDED_REGEX
