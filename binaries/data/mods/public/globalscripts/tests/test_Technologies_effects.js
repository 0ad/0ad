// This tests the GetTechModifiedProperty function.

function test_numeric()
{
	let add = [{ "add": 10, "affects": "Unit" }];

	let add_add = [{ "add": 10, "affects": "Unit" }, { "add": 5, "affects": "Unit" }];

	let add_mul_add = [{ "add": 10, "affects": "Unit" }, { "multiply": 2, "affects": "Unit" }, { "add": 5, "affects": "Unit" }];

	let add_replace = [{ "add": 10, "affects": "Unit" }, { "replace": 10, "affects": "Unit" }];

	let replace_add = [{ "replace": 10, "affects": "Unit" }, { "add": 10, "affects": "Unit" }];

	let replace_replace = [{ "replace": 10, "affects": "Unit" }, { "replace": 30, "affects": "Unit" }];

	TS_ASSERT_EQUALS(GetTechModifiedProperty(add, "Unit", 5), 15);
	TS_ASSERT_EQUALS(GetTechModifiedProperty(add_add, "Unit", 5), 20);
	TS_ASSERT_EQUALS(GetTechModifiedProperty(add_add, "Other", 5), 5);

	// Technologies work by multiplying then adding all.
	TS_ASSERT_EQUALS(GetTechModifiedProperty(add_mul_add, "Unit", 5), 25);

	TS_ASSERT_EQUALS(GetTechModifiedProperty(add_replace, "Unit", 5), 10);

	// Only the first replace is taken into account
	TS_ASSERT_EQUALS(GetTechModifiedProperty(replace_replace, "Unit", 5), 10);
}
test_numeric();

function test_non_numeric()
{
	let replace_nonnum = [{ "replace": "alpha", "affects": "Unit" }];

	TS_ASSERT_EQUALS(GetTechModifiedProperty(replace_nonnum, "Unit", "beta"), "alpha");
	TS_ASSERT_EQUALS(GetTechModifiedProperty(replace_nonnum, "Structure", "beta"), "beta");

	let replace_tokens = [{ "tokens": "-beta alpha gamma -delta", "affects": "Unit" }];
	TS_ASSERT_EQUALS(GetTechModifiedProperty(replace_tokens, "Unit", "beta"), "alpha gamma");
	TS_ASSERT_EQUALS(GetTechModifiedProperty(replace_tokens, "Structure", "beta"), "beta");

	let replace_tokens_2 = [{ "tokens": "beta>gamma -delta", "affects": "Unit" }];
	TS_ASSERT_EQUALS(GetTechModifiedProperty(replace_tokens_2, "Unit", "beta"), "gamma");
	TS_ASSERT_EQUALS(GetTechModifiedProperty(replace_tokens_2, "Structure", "beta"), "beta");

	let replace_tokens_3 = [
		{ "tokens": "beta>alpha gamma", "affects": "Unit" },
		{ "tokens": "alpha>zeta -gamma delta", "affects": "Unit" }
	];
	TS_ASSERT_EQUALS(GetTechModifiedProperty(replace_tokens_3, "Unit", "beta"), "zeta delta");
	TS_ASSERT_EQUALS(GetTechModifiedProperty(replace_tokens_3, "Structure", "beta"), "beta");

	// Ordering matters.
	let replace_tokens_4 = [
		{ "tokens": "alpha>zeta -gamma delta", "affects": "Unit" },
		{ "tokens": "beta>alpha gamma", "affects": "Unit" }
	];
	TS_ASSERT_EQUALS(GetTechModifiedProperty(replace_tokens_4, "Unit", "beta"), "alpha delta gamma");
}

test_non_numeric();
