// TODO: Move this to a folder of tests for GlobalScripts (once one is created)

// This tests the GetTechModifiedProperty function.
let add = [{ "add": 10, "affects": "Unit" }];

let add_add = [{ "add": 10, "affects": "Unit" }, { "add": 5, "affects": "Unit" }];

let add_mul_add = [{ "add": 10, "affects": "Unit" }, { "multiply": 2, "affects": "Unit" }, { "add": 5, "affects": "Unit" }];

let add_replace = [{ "add": 10, "affects": "Unit" }, { "replace": 10, "affects": "Unit" }];

let replace_add = [{ "replace": 10, "affects": "Unit" }, { "add": 10, "affects": "Unit" }];

let replace_replace = [{ "replace": 10, "affects": "Unit" }, { "replace": 30, "affects": "Unit" }];

let replace_nonnum = [{ "replace": "alpha", "affects": "Unit" }];

TS_ASSERT_EQUALS(GetTechModifiedProperty(add, "Unit", 5), 15);
TS_ASSERT_EQUALS(GetTechModifiedProperty(add_add, "Unit", 5), 20);
TS_ASSERT_EQUALS(GetTechModifiedProperty(add_add, "Other", 5), 5);

// Technologies work by multiplying then adding all.
TS_ASSERT_EQUALS(GetTechModifiedProperty(add_mul_add, "Unit", 5), 25);

TS_ASSERT_EQUALS(GetTechModifiedProperty(add_replace, "Unit", 5), 10);

// Only the first replace is taken into account
TS_ASSERT_EQUALS(GetTechModifiedProperty(replace_replace, "Unit", 5), 10);

TS_ASSERT_EQUALS(GetTechModifiedProperty(replace_nonnum, "Unit", "beta"), "alpha");
