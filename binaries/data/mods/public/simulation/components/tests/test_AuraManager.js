Engine.LoadComponentScript("interfaces/AuraManager.js");
Engine.LoadComponentScript("AuraManager.js");

let value = "Component/Value";
let player1 = 1;
let player2 = 2;
let ents1 = [25, 26, 27];
let ents2 = [28, 29, 30];
let ents3 = [31];
let classes = ["class1", "class2"];
let template = { "Identity" : { "Classes" : { "_string" : "class1 class3" } } };

let cmpAuraManager = ConstructComponent(SYSTEM_ENTITY, "AuraManager", {});

// Apply and remove a bonus
cmpAuraManager.ApplyBonus(value, ents1, { "add": 8 }, "key1");
TS_ASSERT_EQUALS(cmpAuraManager.ApplyModifications(value, 10, 25), 18);
// It isn't apply to wrong entity
TS_ASSERT_EQUALS(cmpAuraManager.ApplyModifications(value, 10, 28), 10);
cmpAuraManager.RemoveBonus(value, ents1, "key1");
TS_ASSERT_EQUALS(cmpAuraManager.ApplyModifications(value, 10, 25), 10);

// Apply 2 bonus with two different keys. Bonus should stack
cmpAuraManager.ApplyBonus(value, ents2, { "add": 8 }, "key1");
cmpAuraManager.ApplyBonus(value, ents2, { "multiply": 3 }, "key2");
TS_ASSERT_EQUALS(cmpAuraManager.ApplyModifications(value, 10, 28), 38);

// With another operation ordering, the result must be the same
cmpAuraManager.ApplyBonus(value, ents3, { "multiply": 3 }, "key2");
cmpAuraManager.ApplyBonus(value, ents3, { "add": 8 }, "key1");
TS_ASSERT_EQUALS(cmpAuraManager.ApplyModifications(value, 10, 31), 38);

// Apply bonus to templates
cmpAuraManager.ApplyTemplateBonus(value, player1, classes, { "add": 10 }, "key3");
TS_ASSERT_EQUALS(cmpAuraManager.ApplyTemplateModifications(value, 300, player1, template), 310);
cmpAuraManager.RemoveTemplateBonus(value, player1, classes, "key3");
TS_ASSERT_EQUALS(cmpAuraManager.ApplyTemplateModifications(value, 300, player1, template), 300);
cmpAuraManager.ApplyTemplateBonus(value, player2, classes, { "add": 10 }, "key3");
TS_ASSERT_EQUALS(cmpAuraManager.ApplyTemplateModifications(value, 300, player2, template), 310);
cmpAuraManager.ApplyTemplateBonus(value, player1, classes, { "add": 10 }, "key3");
TS_ASSERT_EQUALS(cmpAuraManager.ApplyTemplateModifications(value, 300, player1, template), 310);
cmpAuraManager.RemoveTemplateBonus(value, player2, classes, "key3");
TS_ASSERT_EQUALS(cmpAuraManager.ApplyTemplateModifications(value, 300, player2, template), 300);
