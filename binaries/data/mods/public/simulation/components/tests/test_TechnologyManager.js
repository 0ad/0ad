Engine.LoadComponentScript("interfaces/TechnologyManager.js");
Engine.LoadComponentScript("TechnologyManager.js");

	AddMock(SYSTEM_ENTITY, IID_DataTemplateManager, {
		"GetAllTechs": () => {}
	});

let cmpTechnologyManager = ConstructComponent(SYSTEM_ENTITY, "TechnologyManager", {});

// Test CheckTechnologyRequirements
let template = { "requirements": { "all": [{ "entity": { "class": "Village", "number": 5 } }, { "civ": "athen" }] } };
cmpTechnologyManager.classCounts["Village"] = 2;
TS_ASSERT_EQUALS(cmpTechnologyManager.CheckTechnologyRequirements(DeriveTechnologyRequirements(template, "athen")), false);
TS_ASSERT_EQUALS(cmpTechnologyManager.CheckTechnologyRequirements(DeriveTechnologyRequirements(template, "athen"), true), true);
TS_ASSERT_EQUALS(cmpTechnologyManager.CheckTechnologyRequirements(DeriveTechnologyRequirements(template, "maur"), true), false);
cmpTechnologyManager.classCounts["Village"] = 6;
TS_ASSERT_EQUALS(cmpTechnologyManager.CheckTechnologyRequirements(DeriveTechnologyRequirements(template, "athen")), true);
TS_ASSERT_EQUALS(cmpTechnologyManager.CheckTechnologyRequirements(DeriveTechnologyRequirements(template, "maur")), false);
