// TODO: Move this to a folder of tests for GlobalScripts (once one is created)

// No requirements set in template
let template = {};
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), []);

/**
 * First, the basics:
 */

// Technology Requirement
template.requirements = { "tech": "expected_tech" };
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), [{ "techs": ["expected_tech"] }]);

// Entity Requirement: Count of entities matching given class
template.requirements = { "entity": { "class": "Village", "number": 5 } };
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), [{ "entities": [{ "class": "Village", "number": 5, "check": "count" }] }]);

// Entity Requirement: Count of entities matching given class
template.requirements = { "entity": { "class": "Village", "numberOfTypes": 5 } };
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), [{ "entities": [{ "class": "Village", "number": 5, "check": "variants" }] }]);

// Single `civ`
template.requirements = { "civ": "athen" };
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), []);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "spart"), false);

// Single `notciv`
template.requirements = { "notciv": "athen" };
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), false);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "spart"), []);


/**
 * Basic `all`s:
 */

// Multiple techs
template.requirements = { "all": [{ "tech": "tech_A" }, { "tech": "tech_B" }, { "tech": "tech_C" }] };
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), [{ "techs": ["tech_A", "tech_B", "tech_C"] }]);

// Multiple entity definitions
template.requirements = {
	"all": [
		{ "entity": { "class": "class_A", "number": 5 } },
		{ "entity": { "class": "class_B", "number": 5 } },
		{ "entity": { "class": "class_C", "number": 5 } }
	]
};
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"),
	[{ "entities": [{ "class": "class_A", "number": 5, "check": "count" }, { "class": "class_B", "number": 5, "check": "count" }, { "class": "class_C", "number": 5, "check": "count" }] }]);

// A `tech` and an `entity`
template.requirements = { "all": [{ "tech": "tech_A" }, { "entity": { "class": "class_B", "number": 5, "check": "count" } }] };
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), [{ "techs": ["tech_A"], "entities": [{ "class": "class_B", "number": 5, "check": "count" }] }]);

// Multiple `civ`s
template.requirements = { "all": [{ "civ": "civ_A"}, { "civ": "civ_B"}, { "civ": "civ_C"}] };
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "civ_A"), []);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "civ_B"), []);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "civ_C"), []);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "civ_D"), false);

// Multiple `notciv`s
template.requirements = { "all": [{ "notciv": "civ_A"}, { "notciv": "civ_B"}, { "notciv": "civ_C"}] };
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "civ_A"), false);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "civ_B"), false);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "civ_C"), false);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "civ_D"), []);

// A `civ` with a tech/entity
template.requirements = { "all": [{ "civ": "athen" }, { "tech": "expected_tech" }] };
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), [{ "techs": ["expected_tech"] }]);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "spart"), false);

template.requirements = { "all": [{ "civ": "athen" }, { "entity": { "class": "Village", "number": 5 } }] };
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), [{ "entities": [{ "class": "Village", "number": 5, "check": "count" }] }]);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "spart"), false);

template.requirements = { "all": [{ "civ": "athen" }, { "entity": { "class": "Village", "numberOfTypes": 5 } }] };
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), [{ "entities": [{ "class": "Village", "number": 5, "check": "variants" }] }]);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "spart"), false);

// A `notciv` with a tech/entity
template.requirements = { "all": [{ "notciv": "athen" }, { "tech": "expected_tech" }] };
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), false);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "spart"), [{ "techs": ["expected_tech"] }]);

template.requirements = { "all": [{ "notciv": "athen" }, { "entity": { "class": "Village", "number": 5 } }] };
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), false);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "spart"), [{ "entities": [{ "class": "Village", "number": 5, "check": "count" }] }]);

template.requirements = { "all": [{ "notciv": "athen" }, { "entity": { "class": "Village", "numberOfTypes": 5 } }] };
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), false);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "spart"), [{ "entities": [{ "class": "Village", "number": 5, "check": "variants" }] }]);


/**
 * Basic `any`s:
 */

// Multiple techs
template.requirements = { "any": [{ "tech": "tech_A" }, { "tech": "tech_B" }, { "tech": "tech_C" }] };
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), [{ "techs": ["tech_A"] }, { "techs": ["tech_B"] }, { "techs": ["tech_C"] }]);

// Multiple entity definitions
template.requirements = {
	"any": [
		{ "entity": { "class": "class_A", "number": 5 } },
		{ "entity": { "class": "class_B", "number": 5 } },
		{ "entity": { "class": "class_C", "number": 5 } }
	]
};
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), [
	{ "entities": [{ "class": "class_A", "number": 5, "check": "count" }] },
	{ "entities": [{ "class": "class_B", "number": 5, "check": "count" }] },
	{ "entities": [{ "class": "class_C", "number": 5, "check": "count" }] }
]);

// A tech or an entity
template.requirements = { "any": [{ "tech": "tech_A" }, { "entity": { "class": "class_B", "number": 5, "check": "count" } }] };
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), [{ "techs": ["tech_A"] }, { "entities": [{ "class": "class_B", "number": 5, "check": "count" }] }]);

// Multiple `civ`s
template.requirements = { "any": [{ "civ": "civ_A" }, { "civ": "civ_B" }, { "civ": "civ_C" }] };
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "civ_A"), []);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "civ_B"), []);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "civ_C"), []);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "civ_D"), false);

// Multiple `notciv`s
template.requirements = { "any": [{ "notciv": "civ_A" }, { "notciv": "civ_B" }, { "notciv": "civ_C" }] };
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "civ_A"), false);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "civ_B"), false);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "civ_C"), false);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "civ_D"), []);

// A `civ` or a tech/entity
template.requirements = { "any": [{ "civ": "athen" }, { "tech": "expected_tech" }] };
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), []);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "spart"), [{ "techs": ["expected_tech"] }]);

template.requirements = { "any": [{ "civ": "athen" }, { "entity": { "class": "Village", "number": 5 } }] };
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), []);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "spart"), [{ "entities": [{ "class": "Village", "number": 5, "check": "count" }] }]);

template.requirements = { "any": [{ "civ": "athen" }, { "entity": { "class": "Village", "numberOfTypes": 5 } }] };
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), []);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "spart"), [{ "entities": [{ "class": "Village", "number": 5, "check": "variants" }] }]);

// A `notciv` or a tech
template.requirements = { "any": [{ "notciv": "athen" }, { "tech": "expected_tech" }] };
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), false);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "spart"), [{ "techs": ["expected_tech"] }]);

template.requirements = { "any": [{ "notciv": "athen" }, { "entity": { "class": "Village", "number": 5 } }] };
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), false);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "spart"), [{ "entities": [{ "class": "Village", "number": 5, "check": "count" }] }]);

template.requirements = { "any": [{ "notciv": "athen" }, { "entity": { "class": "Village", "numberOfTypes": 5 } }] };
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), false);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "spart"), [{ "entities": [{ "class": "Village", "number": 5, "check": "variants" }] }]);


/**
 * Complicated `all`s, part 1 - an `all` inside an `all`:
 */

// Techs
template.requirements = {
	"all": [
		{ "all": [{ "tech": "tech_A" }, { "tech": "tech_B" }] },
		{ "all": [{ "tech": "tech_C" }, { "tech": "tech_D" }] }
	]
};
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), [{ "techs": ["tech_A", "tech_B", "tech_C", "tech_D"] }]);

// Techs and entities
template.requirements = {
	"all": [
		{ "all": [{ "tech": "tech_A" }, { "entity": { "class": "class_A", "number": 5 } }] },
		{ "all": [{ "entity": { "class": "class_B", "numberOfTypes": 5 } }, { "tech": "tech_B" }] }
	]
};
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), [{
	"techs": ["tech_A", "tech_B"],
	"entities": [{ "class": "class_A", "number": 5, "check": "count" }, { "class": "class_B", "number": 5, "check": "variants" }]
}]);

// Two `civ`s, without and with a tech
template.requirements = {
	"all": [
		{ "all": [{ "civ": "athen" }, { "civ": "spart" }] }
	]
};
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), []);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "maur"), false);

template.requirements = {
	"all": [
		{ "tech": "required_tech" },
		{ "all": [{ "civ": "athen" }, { "civ": "spart" }] }
	]
};
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), [{ "techs": ["required_tech"] }]);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "maur"), false);

// Two `notciv`s, without and with a tech
template.requirements = {
	"all": [
		{ "all": [{ "notciv": "athen" }, { "notciv": "spart" }] }
	]
};
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), false);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "maur"), []);

template.requirements = {
	"all": [
		{ "tech": "required_tech" },
		{ "all": [{ "notciv": "athen" }, { "notciv": "spart" }] }
	]
};
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), false);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "maur"), [{ "techs": ["required_tech"] }]);

// Inner `all` has a tech and a `civ`/`notciv`
template.requirements = {
	"all": [
		{ "all": [{ "tech": "tech_A" }, { "civ": "maur" }] },
		{ "tech": "tech_B" }
	]
};
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), [{ "techs": ["tech_B"] }]);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "maur"), [{ "techs": ["tech_A", "tech_B"] }]);

template.requirements = {
	"all": [
		{ "all": [{ "tech": "tech_A" }, { "notciv": "maur" }] },
		{ "tech": "tech_B" }
	]
};
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), [{ "techs": ["tech_A", "tech_B"] }]);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "maur"), [{ "techs": ["tech_B"] }]);


/**
 * Complicated `all`s, part 2 - an `any` inside an `all`:
 */

// Techs
template.requirements = {
	"all": [
		{ "any": [{ "tech": "tech_A" }, { "tech": "tech_B" }] },
		{ "any": [{ "tech": "tech_C" }, { "tech": "tech_D" }] }
	]
};
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), [
	{ "techs": ["tech_A", "tech_C"] },
	{ "techs": ["tech_A", "tech_D"] },
	{ "techs": ["tech_B", "tech_C"] },
	{ "techs": ["tech_B", "tech_D"] }
]);

// Techs and entities
template.requirements = {
	"all": [
		{ "any": [{ "tech": "tech_A" }, { "entity": { "class": "class_A", "number": 5 } }] },
		{ "any": [{ "entity": { "class": "class_B", "numberOfTypes": 5 } }, { "tech": "tech_B" }] }
	]
};
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), [
	{ "techs": ["tech_A"], "entities": [{ "class": "class_B", "number": 5, "check": "variants" }] },
	{ "techs": ["tech_A", "tech_B"] },
	{ "entities": [{ "class": "class_A", "number": 5, "check": "count" }, { "class": "class_B", "number": 5, "check": "variants" }] },
	{ "entities": [{ "class": "class_A", "number": 5, "check": "count" }], "techs": ["tech_B"] }
]);

// Two `civ`s, without and with a tech
template.requirements = {
	"all": [
		{ "any": [{ "civ": "athen" }, { "civ": "spart" }] }
	]
};
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), []);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "maur"), false);

template.requirements = {
	"all": [
		{ "tech": "required_tech" },
		{ "any": [{ "civ": "athen" }, { "civ": "spart" }] }
	]
};
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), [{ "techs": ["required_tech"] }]);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "maur"), false);

// Two `notciv`s, without and with a tech
template.requirements = {
	"all": [
		{ "any": [{ "notciv": "athen" }, { "notciv": "spart" }] }
	]
};
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), false);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "maur"), []);

template.requirements = {
	"all": [
		{ "tech": "required_tech" },
		{ "any": [{ "notciv": "athen" }, { "notciv": "spart" }] }
	]
};
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), false);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "maur"), [{ "techs": ["required_tech"] }]);


/**
 * Complicated `any`s, part 1 - an `all` inside an `any`:
 */

// Techs
template.requirements = {
	"any": [
		{ "all": [{ "tech": "tech_A" }, { "tech": "tech_B" }] },
		{ "all": [{ "tech": "tech_C" }, { "tech": "tech_D" }] }
	]
};
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), [
	{ "techs": ["tech_A", "tech_B"] },
	{ "techs": ["tech_C", "tech_D"] }
]);

// Techs and entities
template.requirements = {
	"any": [
		{ "all": [{ "tech": "tech_A" }, { "entity": { "class": "class_A", "number": 5 } }] },
		{ "all": [{ "entity": { "class": "class_B", "numberOfTypes": 5 } }, { "tech": "tech_B" }] }
	]
};
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), [
	{ "techs": ["tech_A"], "entities": [{ "class": "class_A", "number": 5, "check": "count" }] },
	{ "entities": [{ "class": "class_B", "number": 5, "check": "variants" }], "techs": ["tech_B"] }
]);

// Two `civ`s, without and with a tech
template.requirements = {
	"any": [
		{ "all": [{ "civ": "athen" }, { "civ": "spart" }] }
	]
};
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), []);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "maur"), false);

template.requirements = {
	"any": [
		{ "tech": "required_tech" },
		{ "all": [{ "civ": "athen" }, { "civ": "spart" }] }
	]
};
// Note: these requirements don't really make sense, as the `any` makes the `civ`s in the the inner `all` irrelevant.
// We test it anyway as a precursor to later tests.
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), [{ "techs": ["required_tech"] }]);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "maur"), [{ "techs": ["required_tech"] }]);

// Two `notciv`s, without and with a tech
template.requirements = {
	"any": [
		{ "all": [{ "notciv": "athen" }, { "notciv": "spart" }] }
	]
};
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), false);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "maur"), []);

template.requirements = {
	"any": [
		{ "tech": "required_tech" },
		{ "all": [{ "notciv": "athen" }, { "notciv": "spart" }] }
	]
};
// Note: these requirements have a result that might seen unexpected at first glance.
// This is because the `notciv`s are rendered irrelevant by the `any`, and they have nothing else to operate on.
// We test it anyway as a precursor for later tests.
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), [{ "techs": ["required_tech"] }]);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "maur"), [{ "techs": ["required_tech"] }]);

// Inner `all` has a tech and a `civ`/`notciv`
template.requirements = {
	"any": [
		{ "all": [{ "civ": "civA" }, { "tech": "tech1" }] },
		{ "tech": "tech2" }
	]
};
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "civA"), [{ "techs": ["tech1"] }, { "techs": ["tech2"] }]);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "civB"), [{ "techs": ["tech2"] }]);

template.requirements = {
	"any": [
		{ "all": [{ "notciv": "civA" }, { "tech": "tech1" }] },
		{ "tech": "tech2" }
	]
};
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "civA"), [{ "techs": ["tech2"] }]);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "civB"), [{ "techs": ["tech1"] }, { "techs": ["tech2"] }]);


/**
 * Complicated `any`s, part 2 - an `any` inside an `any`:
 */

// Techs
template.requirements = {
	"any": [
		{ "any": [{ "tech": "tech_A" }, { "tech": "tech_B" }] },
		{ "any": [{ "tech": "tech_C" }, { "tech": "tech_D" }] }
	]
};
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), [
	{ "techs": ["tech_A"] },
	{ "techs": ["tech_B"] },
	{ "techs": ["tech_C"] },
	{ "techs": ["tech_D"] }
]);

// Techs and entities
template.requirements = {
	"any": [
		{ "any": [{ "tech": "tech_A" }, { "entity": { "class": "class_A", "number": 5 } }] },
		{ "any": [{ "entity": { "class": "class_B", "numberOfTypes": 5 } }, { "tech": "tech_B" }] }
	]
};
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), [
	{ "techs": ["tech_A"] },
	{ "entities": [{ "class": "class_A", "number": 5, "check": "count" }] },
	{ "entities": [{ "class": "class_B", "number": 5, "check": "variants" }] },
	{ "techs": ["tech_B"] }
]);

// Two `civ`s, without and with a tech
template.requirements = {
	"any": [
		{ "any": [{ "civ": "athen" }, { "civ": "spart" }] }
	]
};
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), []);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "maur"), false);

template.requirements = {
	"any": [
		{ "tech": "required_tech" },
		{ "any": [{ "civ": "athen" }, { "civ": "spart" }] }
	]
};
// These requirements may not make sense, as the `civ`s are unable to restrict the requirements due to the outer `any`
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), [{ "techs": ["required_tech"] }]);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "maur"), [{ "techs": ["required_tech"] }]);

// Two `notciv`s, without and with a tech
template.requirements = {
	"any": [
		{ "any": [{ "notciv": "athen" }, { "notciv": "spart" }] }
	]
};
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), false);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "maur"), []);

template.requirements = {
	"any": [
		{ "tech": "required_tech" },
		{ "any": [{ "notciv": "athen" }, { "notciv": "spart" }] }
	]
};
// These requirements may not make sense, as the `notciv`s are made irrelevant by the outer `any`
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "athen"), [{ "techs": ["required_tech"] }]);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "maur"), [{ "techs": ["required_tech"] }]);


/**
 * Further tests
 */

template.requirements = {
	"all": [
		{ "tech": "tech1" },
		{ "any": [{ "civ": "civA" }, { "civ": "civB" }] },
		{ "notciv": "civC" }
	]
};
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "civA"), [{ "techs": ["tech1"] }]);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "civC"), false);

template.requirements = {
	"any": [
		{ "all": [{ "civ": "civA" }, { "tech": "tech1" }] },
		{ "all": [{ "civ": "civB" }, { "tech": "tech2" }] }
	]
};
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "civA"), [{ "techs": ["tech1"] }]);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "civB"), [{ "techs": ["tech2"] }]);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "civC"), false);

template.requirements = {
	"any": [
		{ "all": [{ "civ": "civA" }, { "tech": "tech1" }] },
		{ "all": [
			{ "any": [{ "civ": "civB" }, { "civ": "civC" }] },
			{ "tech": "tech2" }
		] }
	]
};
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "civA"), [{ "techs": ["tech1"] }]);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "civC"), [{ "techs": ["tech2"] }]);
TS_ASSERT_UNEVAL_EQUALS(DeriveTechnologyRequirements(template, "civD"), false);

// Test DeriveModificationsFromTech
template = {
	"modifications": [{
		"value": "ResourceGatherer/Rates/food.grain",
		"multiply": 15,
		"affects": "Spearman Swordman"
	},
	{
		"value": "ResourceGatherer/Rates/food.meat",
		"multiply": 10
	}],
	"affects": ["Female", "CitizenSoldier Melee"]
};
let techMods = {
	"ResourceGatherer/Rates/food.grain": [{
		"affects": [
			["Female", "Spearman", "Swordman"],
			["CitizenSoldier", "Melee", "Spearman", "Swordman"]
		],
		"multiply": 15
	}],
	"ResourceGatherer/Rates/food.meat": [{
		"affects": [
			["Female"],
			["CitizenSoldier", "Melee"]
		],
		"multiply": 10
	}]
};
TS_ASSERT_UNEVAL_EQUALS(DeriveModificationsFromTech(template), techMods);

template = {
	"modifications": [{
		"value": "ResourceGatherer/Rates/food.grain",
		"multiply": 15,
		"affects": "Spearman"
	},
	{
		"value": "ResourceGatherer/Rates/food.grain",
		"multiply": 15,
		"affects": "Swordman"
	},
	{
		"value": "ResourceGatherer/Rates/food.meat",
		"multiply": 10
	}],
	"affects": ["Female", "CitizenSoldier Melee"]
};
techMods = {
	"ResourceGatherer/Rates/food.grain": [{
		"affects": [
			["Female", "Spearman"],
			["CitizenSoldier", "Melee", "Spearman"]
		],
		"multiply": 15
	},
	{
		"affects": [
			["Female", "Swordman"],
			["CitizenSoldier", "Melee", "Swordman"]
		],
		"multiply": 15
	}],
	"ResourceGatherer/Rates/food.meat": [{
		"affects": [
			["Female"],
			["CitizenSoldier", "Melee"]
		],
		"multiply": 10
	}]
};
TS_ASSERT_UNEVAL_EQUALS(DeriveModificationsFromTech(template), techMods);
