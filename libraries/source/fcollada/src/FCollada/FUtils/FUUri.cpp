/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FUStringConversion.h"
#include "FUUri.h"

#ifdef WIN32
#define FOLDER_CHAR '\\'
#define UNWANTED_FOLDER_CHAR '/'
#define FOLDER_STR FC("\\")
#else
#define FOLDER_CHAR '/'
#define UNWANTED_FOLDER_CHAR '\\'
#define FOLDER_STR FC("/")
#endif

FUUri::FUUri()
{
	scheme = NONE;
	port = 0;
	path = FC("");
}

FUUri::FUUri(const fstring& uri, bool escape)
	:	scheme(FUUri::NONE),
		port(0),
		path(FC(""))
{
	if (uri.empty()) return;

	fstring _uri;

	if (escape)
	{
		_uri = Escape(uri);
	}
	else
	{
		_uri = uri;
	}

	// Replace all '\\' characters by '/' so the path is only using them
	_uri.replace(FC('\\'), FC('/'));

	// Find the scheme from its ':' delimiter
	size_t schemeDelimiterIndex = _uri.find(FC(':'));
	size_t hostIndex = 0;

	if (schemeDelimiterIndex != fstring::npos && schemeDelimiterIndex > 1)
	{
		fstring _scheme = _uri.substr(0, schemeDelimiterIndex);

		if (IsEquivalent(_scheme, FC("FILE")) || IsEquivalent(_scheme, FC("file")))
		{
			scheme = FUUri::FILE;
		}
		else if (IsEquivalent(_scheme, FC("FTP")) || IsEquivalent(_scheme, FC("ftp")))
		{
			scheme = FUUri::FTP;
		}
		else if (IsEquivalent(_scheme, FC("HTTP")) || IsEquivalent(_scheme, FC("http")))
		{
			scheme = FUUri::HTTP;
		}
		else if (IsEquivalent(_scheme, FC("HTTPS")) || IsEquivalent(_scheme, FC("https")))
		{
			scheme = FUUri::HTTPS;
		}
		else
		{
#ifdef WIN32
			// Scheme not supported (could be a NFS path)
			FUFail(return);
#endif // WIN32
		}

		schemeDelimiter = _uri.substr(schemeDelimiterIndex, 3);
		hostIndex = schemeDelimiterIndex + 3;
	}
	else
	{
#ifdef WIN32
		// Check for windows file path
		if (schemeDelimiterIndex == 1)
		{
			path = FC("/") + _uri;
#else
		// Check for file path
		if (schemeDelimiterIndex == fstring::npos && _uri[0] == (fchar) '/')
		{
			path = _uri;
#endif
			schemeDelimiter = FC("://");

			// We got a file path
			scheme = FUUri::FILE;

			// Check for fragment
			size_t fragmentIndex = path.find(FC('#'));
			if (fragmentIndex != fstring::npos)
			{
				// Extract fragment
				fragment = path.substr(fragmentIndex + 1);
				path = path.substr(0, fragmentIndex);
			}

			// Our URI is parsed
			return;
		}
#ifdef WIN32
		// Check for windows UNC path
		else if (schemeDelimiterIndex == fstring::npos && _uri[0] == '/' && _uri[1] == '/')
		{
			// We got a UNC path
			scheme = FUUri::FILE;
			schemeDelimiter = FC("://");
			hostIndex = 2;
		}
#endif
		else
		{
			// We couldn't detect any scheme
			scheme = FUUri::NONE;
		}
	}

	// Find the hostname from its '/' delimiter. The absence of scheme implies the absence of hostname
	size_t hostDelimiterIndex = 0;
	if (scheme != FUUri::NONE)
	{
		hostDelimiterIndex = _uri.find(FC('/'), hostIndex);

		// If we have a URI, then the first peice is always the host.
		if (hostDelimiterIndex == fstring::npos) hostDelimiterIndex = _uri.length();

		if (hostDelimiterIndex > hostIndex)
		{
			hostname = _uri.substr(hostIndex, hostDelimiterIndex - hostIndex);

			// Check for port
			size_t portIndex = hostname.find(FC(':'));
			if (portIndex != fstring::npos)
			{
				fstring _port = hostname.substr(portIndex + 1);
				port = FUStringConversion::ToInt32(_port);
				hostname = hostname.substr(0, portIndex);
			}

			if (hostname.empty() && (_uri.size() > hostDelimiterIndex + 1) && (_uri[hostDelimiterIndex+1] == '/'))
			{
				// public bug #44 says need file://// for networked paths
				hostIndex++;

				while ((_uri.size() > hostDelimiterIndex + 1) && (_uri[hostDelimiterIndex] == '/'))
				{
					hostDelimiterIndex++;
				}

				size_t realHostDelimiterIndex = _uri.find('/', hostDelimiterIndex);
				if (realHostDelimiterIndex == fstring::npos)
				{
					// This should not happen, so assume that we have a full filename
					scheme = FUUri::FILE;
					path = _uri;
					return;
				}

				hostname = _uri.substr(hostDelimiterIndex, realHostDelimiterIndex - hostDelimiterIndex);
				hostDelimiterIndex = realHostDelimiterIndex;
			}

			// Check for bad URIs that don't include enough slashes.
			if (hostname.size() > 1 && (hostname[1] == ':' || hostname[1] == '|'))
			{
				hostname.clear();
				hostDelimiterIndex = schemeDelimiterIndex + 2;
			}
		}
	}

	// Find the path
	size_t queryDelimiter = _uri.find(FC('?'));
	size_t fragmentDelimiter = _uri.find(FC('#'));

	if (queryDelimiter != fstring::npos) query = _uri.substr(queryDelimiter + 1, fragmentDelimiter - queryDelimiter);
	if (fragmentDelimiter != fstring::npos) fragment = _uri.substr(fragmentDelimiter + 1);

	if (queryDelimiter == fstring::npos && fragmentDelimiter == fstring::npos)
	{
		path = _uri.substr(hostDelimiterIndex);
	}
	else if (queryDelimiter == fstring::npos && fragmentDelimiter != fstring::npos)
	{
		path = _uri.substr(hostDelimiterIndex, fragmentDelimiter - hostDelimiterIndex);
	}
	else
	{
		path = _uri.substr(hostDelimiterIndex, queryDelimiter - hostDelimiterIndex);
	}

	if (path.size() > 1 && path[1] == '|') path[1] = ':';
	else if (path.size() > 2 && path[2] == '|') path[2] = ':';

	if (!IsFile())
	{
		path.append(FC("/"));
	}
}

FUUri::FUUri(Scheme _scheme, const fstring& _user, const fstring& _passwd, const fstring& _host, uint32 _port, const fstring& _path, const fstring& _query, const fstring& _fragment)
	:	scheme(_scheme), 
		username(_user), 
		password(_passwd), 
		hostname(_host), 
		port(_port), 
		path(_path), 
		query(_query), 
		fragment(_fragment)
{
	path.replace(FC('\\'), FC('/'));
	if (path.length() > 2 && path[1] == ':') path.push_front('/');
}

FUUri::FUUri(Scheme _scheme, const fstring& _host, const fstring& _path, const fstring& _fragment)
	:	scheme(_scheme), 
		hostname(_host), 
		port(0), 
		path(_path),
		fragment(_fragment)
{
	path.replace(FC('\\'), FC('/'));
	if (path.length() > 2 && path[1] == ':') path.push_front('/');
}

FUUri::FUUri(const fstring& _path, const fstring& _fragment)
	:	scheme(FUUri::FILE), 
		port(0), 
		path(_path),
		fragment(_fragment)
{
	path.replace(FC('\\'), FC('/'));
	if (path.length() > 2 && path[1] == ':') path.push_front('/');
}

fstring FUUri::GetUserInformations() const
{
	// Return a formated string (user[@password])
	if (username.empty()) return FC("");
	if (password.empty()) return username;
	return username + FC(":") + password;
}

fstring FUUri::GetAuthority() const
{
	fstring authority;
	fstring userInformations = GetUserInformations();

	if (!userInformations.empty())
	{
		authority.append(userInformations);
		authority.append(FC("@"));
	}

	authority.append(hostname);

	if (port != 0)
	{
		authority.append(FC(":"));
		authority.append(TO_FSTRING(FUStringConversion::ToString(port)));
	}

	return authority;
}

fstring FUUri::GetAbsolutePath() const
{
	if (scheme == FUUri::FILE)
	{
		if (GetHostname().empty())
		{
#ifdef WIN32
			// Check if we have a drive letter
			if (path[0] == '/' && path[3] == '/')
			{
				fstring absolutePath;
				fstring uri = path;

				// Replace all '/' by '\\' to be compliant with Windows path
				uri.replace(UNWANTED_FOLDER_CHAR, FOLDER_CHAR);

				// First letter is a drive letter
				fchar driveLetter[2];
				driveLetter[0] = uri[1];
				driveLetter[1] = '\0';
				fstrup(driveLetter);

				absolutePath.append(driveLetter[0]);
				absolutePath.append(uri.substr(2));

				return absolutePath;
			}
			else 
			{
				fstring uri = path.substr(1);
				uri.replace(UNWANTED_FOLDER_CHAR, FOLDER_CHAR);
				return uri;
			}
#else
			return path;
#endif
		}
	}

	fstring outString;

	if (scheme == FUUri::FTP)
	{
		outString.append(FC("ftp"));
	}
	else if (scheme == FUUri::HTTP)
	{
		outString.append(FC("http"));
	}
	else if (scheme == FUUri::HTTPS)
	{
		outString.append(FC("https"));
	}
	
	
	if (scheme == FUUri::FILE)
	{
		if (IsEquivalent(GetHostname(), FC("localhost")))
		{
			outString = path;
			outString.replace(UNWANTED_FOLDER_CHAR, FOLDER_CHAR);
			return outString;
		}
		// UNC scheme
		outString.append(FC("\\\\"));
	}
	else
	{
		outString.append(schemeDelimiter);
	}

	outString.append(GetAuthority());
	outString.append(path);

	if (scheme == FUUri::FILE && !GetHostname().empty())
	{
		outString.replace(UNWANTED_FOLDER_CHAR, FOLDER_CHAR);
	}

	// A query can be necessary to specify the actual file...
	if ((scheme == FUUri::HTTP || scheme == FUUri::HTTPS) 
		&& !GetQuery().empty())
	{
		outString.append(FC("?"));
		outString.append(GetQuery());
	}

	return outString;
}

fstring FUUri::GetAbsoluteUri(bool _fragment) const
{
	fstring outString;

	if (scheme == FUUri::FILE)
	{
		outString.append(FC("file"));
	}
	else if (scheme == FUUri::FTP)
	{
		outString.append(FC("ftp"));
	}
	else if (scheme == FUUri::HTTP)
	{
		outString.append(FC("http"));
	}
	else if (scheme == FUUri::HTTPS)
	{
		outString.append(FC("https"));
	}

	outString.append(schemeDelimiter);
	outString.append(GetAuthority());
	outString.append(path);

	if (!query.empty())
	{
		outString.append(FC("?"));
		outString.append(query);
	}

	if (_fragment && !fragment.empty())
	{
		outString.append(FC("#"));
		outString.append(fragment);
	}

	return outString;
}

fstring FUUri::GetRelativeUri(const FUUri& uri) const
{
	fstring relativePath = uri.MakeRelative(GetAbsolutePath());

	// If we got an absolute uri
	if (relativePath.size() > 0 && relativePath[0] != '.')
	{
		return GetAbsoluteUri();
	}

	if (!query.empty())
	{
		relativePath.append(FC("?"));
		relativePath.append(query);
	}

	if (!fragment.empty())
	{
		relativePath.append(FC("#"));
		relativePath.append(fragment);
	}

#ifdef WIN32
	relativePath.replace(FC('\\'), FC('/'));
#endif // WIN32
	return relativePath;
}

fstring FUUri::MakeRelative(const fstring& _path) const
{
	fstring filePath = _path;

	if (!filePath.empty())
	{
		// First ensure that we have an absolute file path
		filePath = MakeAbsolute(filePath);
		FUUri uri(filePath);
		uri.scheme = GetScheme();

		filePath = uri.GetPath();

		if (!IsEquivalent(GetHostname(), uri.GetHostname()))
		{
			// If it's not the same host so we use the absolute path
			return _path;
		}

		// Relative file path
		FStringList documentPaths, localPaths;
		ExtractPathStack(path, documentPaths, false);
		ExtractPathStack(filePath, localPaths, true);

		if (GetHostname().empty() && GetScheme() == uri.GetScheme() && GetScheme() == FUUri::FILE)
		{
			if (!IsEquivalent(documentPaths.front(), localPaths.front()))
			{
				// We're not on the same drive, return absolute path
				return _path;
			}

#ifdef WIN32
			// Pop drive from the path stacks
			documentPaths.pop_front();
			localPaths.pop_front();
#endif // WIN32

			// If the next folder is different return absolute path
			if (documentPaths.empty() || localPaths.empty() || !IsEquivalent(documentPaths.front(), localPaths.front()))
			{
				return _path;
			}
		}

		// Extract the filename from the stack
		fstring filename = localPaths.back();
		localPaths.pop_back();

		// Look for commonality in the path stacks
		size_t documentPathCount = documentPaths.size();
		size_t filePathCount = localPaths.size();
		size_t matchIndex = 0;

		for (; matchIndex < filePathCount && matchIndex < documentPathCount; ++matchIndex)
		{
			if (!IsEquivalent(documentPaths[matchIndex], localPaths[matchIndex])) break;
		}

		if (matchIndex > 0)
		{
			// There are some similar parts, so generate the relative filename
			fstring relativePath;

			if (documentPathCount > matchIndex)
			{
				// Backtrack the document's path
				for (size_t i = matchIndex; i < documentPathCount; ++i)
				{
					relativePath += FC("../");
				}
			}
			else
			{
				relativePath = FC("./");
			}

			// Add the file's relative path
			for (size_t i = matchIndex; i < filePathCount; ++i)
			{
				relativePath += localPaths[i] + FC("/");
			}

			filePath = relativePath + filename;
		}
	}

	filePath.replace(UNWANTED_FOLDER_CHAR, FOLDER_CHAR);

	return filePath;
}

fstring FUUri::MakeAbsolute(const fstring& relativePath) const
{
	if (relativePath.empty()) return relativePath;
	FUUri uri(relativePath);

	MakeAbsolute(uri);
	return uri.GetAbsolutePath();
}

void FUUri::MakeAbsolute(FUUri& uri) const
{
	fstring filePath = uri.GetPath();

	if (uri.GetScheme() != FUUri::NONE)
	{
		return;
	}
	else
	{
		// Since path is relative to this one
		uri.scheme = this->scheme;
		uri.schemeDelimiter = this->schemeDelimiter;
		uri.hostname = this->hostname;
		uri.port = this->port;
	}

#ifdef WIN32
	if (filePath.size() > 1 && filePath[1] == '|') filePath[1] = ':';
#endif // WIN32


	if (uri.scheme == FILE || uri.scheme == NONE)
	{
		if ((!filePath.empty() && (filePath[0] == '\\' || filePath[0] == '/')) || (filePath.size() > 1 && filePath[1] == ':'))
		{
#ifdef WIN32
			// In win32 we need to add the drive to the path
			if (path.size() > 1)
			{
				uri.path = path.substr(0, 3);
				uri.path.append(filePath);

				uri.path.replace(FC('\\'), FC('/'));
			}
#endif
			
			// Path is already absolute
			return;
		}
	}

	if (scheme == HTTP)
	{
		// path is already absolute.
		if (filePath[0] == '/')
		{
			return;
		}
	}


	// Relative file path
	FStringList documentPaths, localPaths;
	ExtractPathStack(path, documentPaths, false);
	ExtractPathStack(filePath, localPaths, true);

	for (FStringList::iterator it = localPaths.begin(); it != localPaths.end(); ++it)
	{
		if ((*it) == FC(".")) {}	// Do nothing
		else if ((*it) == FC(".."))
		{
			// Pop one path out
			if (!documentPaths.empty()) documentPaths.pop_back();
		}
		else // Traverse this path
		{
			documentPaths.push_back(*it);
		}
	}

	// Recreate the absolute filename
	fstring outPath;

	for (FStringList::iterator it = documentPaths.begin(); it != documentPaths.end(); ++it)
	{
		outPath.append(FC('/'));
		outPath.append(*it);
	}

	uri.path = outPath;
}

FUUri FUUri::Resolve(const fstring& relativePath) const
{
	fstring absolute = MakeAbsolute(relativePath);
	return FUUri(absolute);
}

bool FUUri::IsFile() const
{
	return path.length() > 1 && path.back() != '/';
}

bool FUUri::IsAlpha(fchar fc)
{
	return (fc >= 'A' && fc <= 'Z') || (fc >= 'a' && fc <= 'z');
}

bool FUUri::IsDigit(fchar fc)
{
	return fc >= '0' && fc <= '9';
}

bool FUUri::IsAlphaNumeric(fchar fc)
{
	return IsAlpha(fc) || IsDigit(fc);
}

bool FUUri::IsMark(fchar fc)
{
	return (fc == '-' || fc == '_' || fc == '.' || fc == '!' || fc == '~' || fc == '*' || fc == '\'' || fc == '(' || fc == ')');
}

bool FUUri::IsHex(fchar fc)
{
	// Hexadecimal digits (0-9, a-f, and A-F).
	return (fc >= 'A' && fc <= 'F') || (fc >= 'a' && fc <= 'f') || (fc >= '0' && fc <= '9');
}

bool FUUri::IsReserved(fchar fc)
{
	return (fc == ';' || fc == '/' || fc == '?' || fc == ':' || fc == '@' || fc == '&' || fc == '=' || fc == '+' || fc == '$' || fc == ',');
}

fstring FUUri::Escape(const fstring& path)
{
	fstring escaped;

	for (fstring::const_iterator it = path.begin(); it != path.end(); ++it)
	{
		if (IsAlphaNumeric(*it) || IsMark(*it) || IsReserved(*it))
		{
			// Character it not escaped
			escaped.push_back(*it);
		}

		if ((*it) == '%')
		{
			// Check if it is followed by an hex character
			fstring::const_iterator itNext = it;
			itNext++;

			if (IsHex(*itNext))
			{
				// Character it not escaped
				escaped.push_back(*it);
			}
		}

		// Escape current character
		fstring _escaped = TO_FSTRING(FUStringConversion::ToString(int(*it)));
		escaped.push_back(FC('%'));
		escaped.append(_escaped.c_str());
	}

	return escaped;
}

// For a relative path, extract the list of the individual paths that must be traversed to get to the file.
void FUUri::ExtractPathStack(const fstring& name, FStringList& list, bool includeFilename) const
{
	list.clear();
	list.reserve(6);

	fstring split = name;
	split.replace(FC('\\'), FC('/'));

	while (!split.empty())
	{
		// Extract out the next path
		size_t slashIndex = split.find('/');
		if (slashIndex != fstring::npos && slashIndex != 0)
		{
			list.push_back(split.substr(0, slashIndex));
			split.erase(0, slashIndex + 1);
		}
		else if (slashIndex != 0)
		{
			if (includeFilename) list.push_back(split);
			split.clear();
		}
		else
		{
			split.erase(0, 1);
		}
	}
}
