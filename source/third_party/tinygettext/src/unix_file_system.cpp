//  tinygettext - A gettext replacement that works directly on .po files
//  Copyright (C) 2009 Ingo Ruhnke <grumbel@gmx.de>
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include "precompiled.h"

#include "tinygettext/unix_file_system.hpp"

#include <sys/types.h>
#include <fstream>
#ifdef _MSC_VER
// MSVC doesn't include dirent.h, so we use this emulated win32 version
#  include "win32/dirent.h"
#else
#  include <dirent.h>
#endif
#include <stdlib.h>
#include <string.h>

namespace tinygettext {

UnixFileSystem::UnixFileSystem()
{
}

std::vector<std::string>
UnixFileSystem::open_directory(const std::string& pathname)
{
  DIR* dir = opendir(pathname.c_str());
  if (!dir)
  {
    // FIXME: error handling
    return std::vector<std::string>();
  }
  else
  {
    std::vector<std::string> files;

    struct dirent* dp;
    while((dp = readdir(dir)) != 0)
    {
      files.push_back(dp->d_name);
    }
    closedir(dir);

    return files;
  }
}

std::unique_ptr<std::istream>
UnixFileSystem::open_file(const std::string& filename)
{
  return std::unique_ptr<std::istream>(new std::ifstream(filename.c_str()));
}

} // namespace tinygettext

/* EOF */
