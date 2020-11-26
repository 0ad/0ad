/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FUBoundingBox.h"
#include "FUBoundingSphere.h"
#include "FUTestBed.h"

TESTSUITE_START(FUBoundingTest)

TESTSUITE_TEST(0, BoundingBox)
	// Create a simple bounding box and verify point containment.
	FUBoundingBox box;
	FailIf(box.IsValid());
	box.SetMin(FMVector3(10.0f, 10.0f, 10.0f));
	box.SetMax(FMVector3(14.0f, 13.0f, 15.0f));
	PassIf(box.IsValid());
	PassIf(box.Contains(FMVector3(10.0f, 11.0f, 12.0f)));
	PassIf(box.Contains(FMVector3(10.0f, 13.0f, 15.0f)));
	FailIf(box.Contains(FMVector3(0.0f, 1.0f, 2.0f)));
	FailIf(box.Contains(FMVector3(10.0f, 14.0f, 12.0f)));

	// Create a second bounding box.
	// Verify the behavior of invalid bounding boxes.
	FUBoundingBox box2;
	FailIf(box2.IsValid());
	FailIf(box.Overlaps(box2));
	FailIf(box2.Overlaps(box));
	FailIf(box2.Contains(FMVector3(0.0f, 0.0f, 0.0f)));
	FailIf(box2.Contains(FMVector3(FLT_MAX, FLT_MAX, FLT_MAX)));

	// Initialize the second bounding box and verify overlap.
	box2.SetMin(FMVector3(0.0f, 0.0f, 0.0f));
	box2.SetMax(FMVector3(9.0f, 9.0f, 9.0f));
	PassIf(box2.IsValid());
	FailIf(box.Overlaps(box2));
	FailIf(box2.Overlaps(box));

	// Create a third bounding box and verify overlap.
	FUBoundingBox box3(FMVector3(8.0f, 8.0f, 12.0f), FMVector3(12.0f, 12.0f, 14.0f));
	PassIf(box3.IsValid());
	FailIf(box3.Overlaps(box2));
	FailIf(box2.Overlaps(box3));
	PassIf(box3.Overlaps(box));
	PassIf(box.Overlaps(box3));

	// Include a point and verify its containment
	box3.Include(FMVector3(0.0f, 0.0f, 20.0f));
	PassIf(box3.Contains(FMVector3(0.0f, 0.0f, 20.0f)));
	PassIf(box3.Contains(FMVector3(1.0f, 1.0f, 19.0f)));

TESTSUITE_TEST(1, BoundingSphere);
	// Create a simple bounding sphere and verify point containment.
	FUBoundingSphere sphere;
	FailIf(sphere.IsValid());
	sphere.SetCenter(FMVector3(5.0f, 10.0f, 0.0f));
	sphere.SetRadius(5.0f);
	PassIf(sphere.IsValid());
	PassIf(sphere.Contains(FMVector3(0.0f, 10.0f, 0.0f)));
	PassIf(sphere.Contains(FMVector3(6.0f, 9.0f, 1.0f)));
	FailIf(sphere.Contains(FMVector3(5.0f, 6.0f, 4.0f)));

	// Create a second bounding sphere and verify the invalid sphere behavior.
	FUBoundingSphere sphere2;
	FailIf(sphere2.IsValid());
	FailIf(sphere2.Contains(FMVector3(0.0f, 0.0f, 0.0f)));
	FailIf(sphere2.Contains(FMVector3(1.0f, -5.0f, 4.0f)));
	FailIf(sphere.Overlaps(sphere2));
	FailIf(sphere2.Overlaps(sphere));

	// Initialize the second bounding sphere and verify overlap.
	sphere2.SetCenter(FMVector3(11.0f, 10.0f, 0.0f));
	sphere2.SetRadius(0.5f);
	PassIf(sphere2.IsValid());
	FailIf(sphere.Overlaps(sphere2));
	FailIf(sphere2.Overlaps(sphere));
	sphere2.SetRadius(2.0f);
	PassIf(sphere.Overlaps(sphere2));
	PassIf(sphere2.Overlaps(sphere));

	// Test overlap using a third sphere.
	FUBoundingSphere sphere3(FMVector3(1.0f, 6.0f, 4.0f), 7.0f);
	PassIf(sphere3.IsValid());
	FailIf(sphere3.Overlaps(sphere2));
	FailIf(sphere2.Overlaps(sphere3));
	PassIf(sphere.Overlaps(sphere3));
	PassIf(sphere3.Overlaps(sphere));

	// Test overlap using a bounding box.
	FUBoundingBox box(FMVector3(5.0f, 10.0f, -8.0f), FMVector3(12.0f, 12.0f, 0.0f));
	PassIf(box.Overlaps(sphere3));
	PassIf(sphere3.Overlaps(box));
	PassIf(box.Overlaps(sphere2));
	PassIf(sphere2.Overlaps(box));
	PassIf(sphere.Overlaps(box));
	PassIf(box.Overlaps(sphere));
	FUBoundingBox box2(FMVector3(-5.0f, -10.0f, 8.0f), FMVector3(-4.0f, -8.0f, 10.0f));
	FailIf(box2.Overlaps(sphere3));
	FailIf(sphere3.Overlaps(box2));
	FailIf(box2.Overlaps(sphere2));
	FailIf(sphere2.Overlaps(box2));
	FailIf(sphere.Overlaps(box2));
	FailIf(box2.Overlaps(sphere));

	// Special case: test where no vertices is inside the sphere or box, but they overlap still!
	FUBoundingBox box3(FMVector3(-10.0f, -10.0f, -10.0f), FMVector3(10.0f, 10.0f, 3.0f));
	PassIf(box3.Overlaps(sphere3));
	PassIf(sphere3.Overlaps(box3));

	// Include a point in the third sphere and verify its containment
	sphere3.Include(FMVector3(0.0f, 0.0f, 0.0f));
	PassIf(sphere3.Contains(FMVector3(0.0f, 0.0f, 0.0f)));

TESTSUITE_END
