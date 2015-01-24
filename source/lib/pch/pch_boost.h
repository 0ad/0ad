#ifndef INCLUDED_PCH_BOOST
#define INCLUDED_PCH_BOOST

#include "lib/external_libraries/suppress_boost_warnings.h"

#if MSC_VERSION
# define BOOST_HAS_STDINT_H
#endif

// Boost
// .. if this package isn't going to be statically linked, we're better off
// using Boost via DLL. (otherwise, we would have to ensure the exact same
// compiler is used, which is a pain because MSC8, MSC9 and ICC 10 are in use)
#ifndef LIB_STATIC_LINK
# define BOOST_ALL_DYN_LINK
#endif

// don't compile get_system_category() etc, since we don't use them and they
// sometimes cause problems when linking.
// But Filesystem <= 1.43 requires boost::system::posix, so only disable if newer
#include <boost/version.hpp>
#if BOOST_VERSION >= 104400
# define BOOST_SYSTEM_NO_DEPRECATED
#endif

// the following boost libraries have been included in TR1 and are
// thus deemed usable:
#if BOOST_VERSION >= 104400
// Filesystem v3 is included since Boost 1.44
// v2 is deprecated since 1.46 and removed entirely in 1.50
# define BOOST_FILESYSTEM_VERSION 3
#else
# define BOOST_FILESYSTEM_VERSION 2
#endif
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#include <boost/shared_ptr.hpp>

// (these ones are used more rarely, so we don't enable them in minimal configurations)
#if !MINIMAL_PCH
#include <boost/array.hpp>
using boost::array;

#include <boost/mem_fn.hpp>
using boost::mem_fn;

#include <boost/function.hpp>
using boost::function;

#include <boost/bind.hpp>
using boost::bind;
#endif // !MINIMAL_PCH

#endif	// #ifndef INCLUDED_PCH_BOOST
