/**
 * =========================================================================
 * File        : boost_filesystem.h
 * Project     : 0 A.D.
 * Description : bring in Boost filesystem library
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_BOOST_FILESYSTEM
#define INCLUDED_BOOST_FILESYSTEM

// not W4-clean
#if MSC_VERSION
# pragma warning(push, 3)
#endif

#include "boost/filesystem.hpp"
namespace fs = boost::filesystem;

#if MSC_VERSION
# pragma warning(pop)
#endif

#endif	// #ifndef INCLUDED_BOOST_FILESYSTEM
