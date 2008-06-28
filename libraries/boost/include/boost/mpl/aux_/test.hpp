
#ifndef BOOST_MPL_AUX_TEST_HPP_INCLUDED
#define BOOST_MPL_AUX_TEST_HPP_INCLUDED

// Copyright Aleksey Gurtovoy 2002-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Source$
// $Date: 2006-02-28 17:56:33 -0500 (Tue, 28 Feb 2006) $
// $Revision: 33181 $

#include <boost/mpl/aux_/test/test_case.hpp>
#include <boost/mpl/aux_/test/data.hpp>
#include <boost/mpl/aux_/test/assert.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/type_traits/is_same.hpp>

int main()
{
    return boost::report_errors();
}

using namespace boost;
using namespace mpl;

#endif // BOOST_MPL_AUX_TEST_HPP_INCLUDED
