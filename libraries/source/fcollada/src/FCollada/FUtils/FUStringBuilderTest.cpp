/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FUTestBed.h"
#include "FUString.h"

TESTSUITE_START(FUStringBuilder)

TESTSUITE_TEST(0, Creation_Equivalence)
	FUSStringBuilder builder;
	PassIf(IsEquivalent(builder, ""));
	FUSStringBuilder builder1("55qa");
	PassIf(IsEquivalent(builder1, "55qa"));
	FUSStringBuilder builder2('c', 1);
	PassIf(IsEquivalent(builder2, "c"));
	FUSStringBuilder builder3(5);
	PassIf(IsEquivalent(builder3, ""));

TESTSUITE_TEST(1, Modifications)
	FUSStringBuilder builder;
	builder.append((int32) 12);
	PassIf(IsEquivalent(builder, "12"));
	builder.append("  aa");
	PassIf(IsEquivalent(builder, "12  aa"));
	builder.remove(4);
	PassIf(IsEquivalent(builder, "12  "));
	builder.append('s');
	PassIf(IsEquivalent(builder, "12  s"));
	builder.append(55.55);
	PassIf(strstr(builder.ToCharPtr(), "12  s55.55") != NULL);
	builder.clear();
	PassIf(IsEquivalent(builder, ""));
	builder.pop_back();
	PassIf(IsEquivalent(builder, ""));
	builder.append(677u);
	PassIf(IsEquivalent(builder, "677"));
	builder.pop_back();
	PassIf(IsEquivalent(builder, "67"));
	builder.remove(0, 1);
	PassIf(IsEquivalent(builder, "7"));

TESTSUITE_TEST(2, ExtremeNumbers)
	FUSStringBuilder builder;

	builder.set(-10231.52f);
	// Last time I checked: we had 6 digits of precision.
	// If this one fails: the number of digits of precisions have changed
	// and the strings below are now useless.
	PassIf(IsEquivalent(builder.ToCharPtr(), "-10231.5"));

	builder.set(123456789.0f);
	PassIf(IsEquivalent(builder.ToCharPtr(), "1.23457e8"));

	builder.set(1e16f);
	PassIf(IsEquivalent(builder.ToCharPtr(), "1e16"));

	builder.set(-1e16f);
	PassIf(IsEquivalent(builder.ToCharPtr(), "-1e16"));

	builder.set(-1.5e10f);
	PassIf(IsEquivalent(builder.ToCharPtr(), "-1.5e10"));

	builder.set(9.55e9f);
	PassIf(IsEquivalent(builder.ToCharPtr(), "9.55e9"));

	// Right now, the expected behavior for very small numbers is to output a zero.
	builder.set(-1e-16f);
	PassIf(IsEquivalent(builder.ToCharPtr(), "0"));

TESTSUITE_END
