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

#include "precompiled.h"

#include "lib/sysdep/sysdep.h"
#include "lib/external_libraries/boost_filesystem.h"

#define GNU_SOURCE
#include "mocks/dlfcn.h"
#include "mocks/boost_filesystem.h"

// TODO: This normalization code is copied from boost::filesystem,
// where it is deprecated, presumably for good reasons (probably
// because symlinks mean "x/../y" != "y" in general), and this file
// is not an appropriate place for the code anyway, so it should be
// rewritten or removed or something. (Also, the const_cast is evil.)
namespace boost { namespace filesystem {

// Derived from boost/filesystem/path.hpp:
// "Copyright Beman Dawes 2002-2005
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)"

    template<class String, class Traits>
    basic_path<String, Traits> normalize_COPIED(const basic_path<String, Traits> &self)
    {
      typedef basic_path<String, Traits> path_type;
      typedef typename path_type::string_type string_type;
      typedef typename path_type::iterator iterator;

      static const typename string_type::value_type dot_str[]
        = { dot<path>::value, 0 };

      if ( self.empty() ) return self;
        
      path temp;
      iterator start( self.begin() );
      iterator last( self.end() );
      iterator stop( last-- );
      for ( iterator itr( start ); itr != stop; ++itr )
      {
        // ignore "." except at start and last
        if ( itr->size() == 1
          && (*itr)[0] == dot<path_type>::value
          && itr != start
          && itr != last ) continue;

        // ignore a name and following ".."
        if ( !temp.empty()
          && itr->size() == 2
          && (*itr)[0] == dot<path_type>::value
          && (*itr)[1] == dot<path_type>::value ) // dot dot
        {
          string_type lf( temp.leaf() );  
          if ( lf.size() > 0  
            && (lf.size() != 1
              || (lf[0] != dot<path_type>::value
                && lf[0] != slash<path_type>::value))
            && (lf.size() != 2 
              || (lf[0] != dot<path_type>::value
                && lf[1] != dot<path_type>::value
#             ifdef BOOST_WINDOWS_PATH
                && lf[1] != colon<path_type>::value
#             endif
                 )
               )
            )
          {
            temp.remove_leaf();
            // if not root directory, must also remove "/" if any
            if ( temp.string().size() > 0
              && temp.string()[temp.string().size()-1]
                == slash<path_type>::value )
            {
              typename string_type::size_type rds(
                detail::root_directory_start<String,Traits>( temp.string(),
                  temp.string().size() ) );
              if ( rds == string_type::npos
                || rds != temp.string().size()-1 ) 
                { const_cast<String&>( temp.string() ).erase( temp.string().size()-1 ); }
            }

            iterator next( itr );
            if ( temp.empty() && ++next != stop
              && next == last && *last == dot_str ) temp /= dot_str;
            continue;
          }
        }

        temp /= *itr;
      };

      if ( temp.empty() ) temp /= dot_str;
      return temp;
    }

} }

LibError sys_get_executable_name(char* n_path, size_t max_chars)
{
	const char* path;
	Dl_info dl_info;

	// Find the executable's filename
	memset(&dl_info, 0, sizeof(dl_info));
	if (!T::dladdr((void *)sys_get_executable_name, &dl_info) ||
		!dl_info.dli_fname )
	{
		return ERR::NO_SYS;
	}
	path = dl_info.dli_fname;

	// If this looks like a relative path, resolve against cwd.
	// If this looks like an absolute path, we still need to normalize it.
	if (strchr(path, '/')) {
		fs::path p = fs::complete(fs::path(path), T::Boost_Filesystem_initial_path());
		fs::path n = fs::normalize_COPIED(p);
		strncpy(n_path, n.string().c_str(), max_chars);
		return INFO::OK;
	}

	// If it's not a path at all, i.e. it's just a filename, we'd
	// probably have to search through PATH to find it.
	// That's complex and should be uncommon, so don't bother.
	return ERR::NO_SYS;
}
