/*
 * Slightly modified version by Wildfire Games, for 0 A.D to support macOS 10.13
 */
// tinygettext - A gettext replacement that works directly on .po files
// Copyright (c) 2009 Ingo Ruhnke <grumbel@gmail.com>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgement in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#include "precompiled.h"

#include "tinygettext/unix_file_system.hpp"

#include <boost/filesystem.hpp>
#include <fstream>
#include <stdlib.h>

namespace tinygettext {

UnixFileSystem::UnixFileSystem()
{
}

std::vector<std::string>
UnixFileSystem::open_directory(const std::string& pathname)
{
  std::vector<std::string> files;
  for(auto const& p : boost::filesystem::directory_iterator(pathname))
  {
    files.push_back(p.path().filename().string());
  }
  return files;
}

std::unique_ptr<std::istream>
UnixFileSystem::open_file(const std::string& filename)
{
  return std::unique_ptr<std::istream>(new std::ifstream(filename));
}

} // namespace tinygettext

/* EOF */
