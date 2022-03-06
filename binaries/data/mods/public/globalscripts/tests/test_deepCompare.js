function test_deepCompare()
{
	TS_ASSERT(deepCompare({}, {}));
	TS_ASSERT(deepCompare([], []));
	TS_ASSERT(deepCompare({ "foo": NaN }, { "foo": NaN }));
	TS_ASSERT(!deepCompare({ "foo": Infinity }, { "foo": NaN }));
	TS_ASSERT(!deepCompare({ "foo": NaN }, { "foo": Infinity }));
	TS_ASSERT(!deepCompare({ "foo": NaN }, { "bar": NaN }));
	TS_ASSERT(!deepCompare({ "foo": NaN }, { "foo": NaN, "bar": NaN }));
	TS_ASSERT(!deepCompare({ "foo": NaN, "bar": NaN }, { "foo": NaN }));

	TS_ASSERT(deepCompare(undefined, undefined));
	TS_ASSERT(deepCompare([undefined], [undefined]));
	TS_ASSERT(deepCompare({ "foo": undefined }, { "foo": undefined }));
	TS_ASSERT(!deepCompare({ "foo": undefined }, {}));

	// Ordering in objects does not matter.
	TS_ASSERT(deepCompare({ "foo": NaN, "bar": NaN }, { "foo": NaN, "bar": NaN  }));
	TS_ASSERT(deepCompare({ "foo": NaN, "bar": NaN }, { "bar": NaN, "foo": NaN  }));

	// Test some other JS structures.
	TS_ASSERT(deepCompare(new Set(), new Set()));
	TS_ASSERT(deepCompare(new Map(), new Map()));
	TS_ASSERT(!deepCompare(new Set(), new Map()));
	TS_ASSERT(!deepCompare(new Set([0]), new Set([1])));
	TS_ASSERT(!deepCompare(new Set([undefined]), new Set([null])));
	TS_ASSERT(deepCompare(new Set([NaN]), new Set([NaN])));
	TS_ASSERT(deepCompare(new Set([0, 0, 0]), new Set([0])));

	// Ordering in arrays is relevant.
	TS_ASSERT(deepCompare([1, 2, 3], [1, 2, 3]));
	TS_ASSERT(!deepCompare([1, 2, 3], [3, 1, 2]));

	// Some nestling.
	TS_ASSERT(deepCompare({ "foo": new Set([1, 2, { "baz": Infinity }]), "bar": [new Set([9]), { "foo": [0, 1, 2] }] },
	                      { "foo": new Set([1, 2, { "baz": Infinity }]), "bar": [new Set([9]), { "foo": [0, 1, 2] }] }));
}

test_deepCompare();
