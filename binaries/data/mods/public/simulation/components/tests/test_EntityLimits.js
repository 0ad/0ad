Engine.LoadComponentScript("interfaces/BuildRestrictions.js");
Engine.LoadComponentScript("interfaces/EntityLimits.js");
Engine.LoadComponentScript("interfaces/Foundation.js");
Engine.LoadComponentScript("interfaces/TrainingRestrictions.js");
Engine.LoadComponentScript("interfaces/Player.js");
Engine.LoadComponentScript("EntityLimits.js");

let template ={
	"Limits": {
		"Tower": 5,
		"Wonder": 1,
		"Hero": 2,
		"Champion": 1
	},
	"LimitChangers": {
		"Tower": { "Monument": 1 }
	},
	"LimitRemovers": {
		"Tower": { "RequiredTechs": { "_string": "TechA" } },
		"Hero": { "RequiredClasses": { "_string": "Aegis" } }
	}
};

AddMock(10, IID_Player, {
	"GetPlayerID": id => 1
});
AddMock(SYSTEM_ENTITY, IID_GuiInterface, {
	"PushNotification": () => {}
});

let cmpEntityLimits = ConstructComponent(10, "EntityLimits", template);

// Test getters
TS_ASSERT_UNEVAL_EQUALS(cmpEntityLimits.GetCounts(), { "Tower": 0, "Wonder": 0, "Hero": 0, "Champion": 0 });
TS_ASSERT_UNEVAL_EQUALS(cmpEntityLimits.GetLimits(), { "Tower": 5, "Wonder": 1, "Hero": 2, "Champion": 1 });
TS_ASSERT_UNEVAL_EQUALS(cmpEntityLimits.GetLimitChangers(), { "Tower": { "Monument": 1 } });

// Test training restrictions
TS_ASSERT(cmpEntityLimits.AllowedToTrain("Hero"));
TS_ASSERT(cmpEntityLimits.AllowedToTrain("Hero", 1));
TS_ASSERT(cmpEntityLimits.AllowedToTrain("Hero", 2));

for (let ent = 60; ent < 63; ++ent)
{
	AddMock(ent, IID_TrainingRestrictions, {
		"GetCategory": () => "Hero"
});
}

cmpEntityLimits.OnGlobalOwnershipChanged({ "entity": 60, "from": -1, "to": 1 });
cmpEntityLimits.OnGlobalOwnershipChanged({ "entity": 61, "from": 2, "to": 1 });

TS_ASSERT_UNEVAL_EQUALS(cmpEntityLimits.GetCounts(), { "Tower": 0, "Wonder": 0, "Hero": 2, "Champion": 0 });
TS_ASSERT(cmpEntityLimits.AllowedToTrain("Hero"));
TS_ASSERT(!cmpEntityLimits.AllowedToTrain("Hero", 1));

// Restrictions can be enforced
cmpEntityLimits.OnGlobalOwnershipChanged({ "entity": 62, "from": -1, "to": 1 });
TS_ASSERT_UNEVAL_EQUALS(cmpEntityLimits.GetCounts(), { "Tower": 0, "Wonder": 0, "Hero": 3, "Champion": 0 });

for (let ent = 60; ent < 63; ++ent)
	cmpEntityLimits.OnGlobalOwnershipChanged({ "entity": ent, "from": 1, "to": -1 });

TS_ASSERT_UNEVAL_EQUALS(cmpEntityLimits.GetCounts(), { "Tower": 0, "Wonder": 0, "Hero": 0, "Champion": 0 });

// Test building restrictions
AddMock(70, IID_BuildRestrictions, {
	"GetCategory": () => "Wonder"
});

cmpEntityLimits.OnGlobalOwnershipChanged({ "entity": 70, "from": 3, "to": 1 });
TS_ASSERT_UNEVAL_EQUALS(cmpEntityLimits.GetCounts(), { "Tower": 0, "Wonder": 1, "Hero": 0, "Champion": 0 });
TS_ASSERT_UNEVAL_EQUALS(cmpEntityLimits.GetLimits(), { "Tower": 5, "Wonder": 1, "Hero": 2, "Champion": 1 });

// AllowedToBuild is used after foundation placement, which are meant to be replaced
TS_ASSERT(cmpEntityLimits.AllowedToBuild("Wonder"));

cmpEntityLimits.OnGlobalOwnershipChanged({ "entity": 70, "from": 1, "to": -1 });

// Test limit changers
AddMock(80, IID_Identity, {
	"GetClassesList": () => ["Monument"]
});

AddMock(81, IID_Identity, {
	"GetClassesList": () => ["Monument"]
});

cmpEntityLimits.OnGlobalOwnershipChanged({ "entity": 80, "from": -1, "to": 1 });

TS_ASSERT_UNEVAL_EQUALS(cmpEntityLimits.GetCounts(), { "Tower": 0, "Wonder": 0, "Hero": 0, "Champion": 0 });
TS_ASSERT_UNEVAL_EQUALS(cmpEntityLimits.GetLimits(), { "Tower": 5 + 1, "Wonder": 1, "Hero": 2, "Champion": 1 });

cmpEntityLimits.OnGlobalOwnershipChanged({ "entity": 81, "from": 1, "to": -1 });

TS_ASSERT_UNEVAL_EQUALS(cmpEntityLimits.GetCounts(), { "Tower": 0, "Wonder": 0, "Hero": 0, "Champion": 0 });
TS_ASSERT_UNEVAL_EQUALS(cmpEntityLimits.GetLimits(), { "Tower": 5, "Wonder": 1, "Hero": 2, "Champion": 1 });

// Foundations don't change limits
AddMock(81, IID_Foundation, {});

cmpEntityLimits.OnGlobalOwnershipChanged({ "entity": 81, "from": -1, "to": 1 });

TS_ASSERT_UNEVAL_EQUALS(cmpEntityLimits.GetCounts(), { "Tower": 0, "Wonder": 0, "Hero": 0, "Champion": 0 });
TS_ASSERT_UNEVAL_EQUALS(cmpEntityLimits.GetLimits(), { "Tower": 5, "Wonder": 1, "Hero": 2, "Champion": 1 });

cmpEntityLimits.OnGlobalOwnershipChanged({ "entity": 81, "from": 1, "to": -1 });

// Test limit removers by classes
AddMock(90, IID_Identity, {
	"GetClassesList": () => ["Aegis"]
});

cmpEntityLimits.OnGlobalOwnershipChanged({ "entity": 90, "from": -1, "to": 1 });

TS_ASSERT_UNEVAL_EQUALS(cmpEntityLimits.GetLimits(), { "Tower": 5, "Wonder": 1, "Hero": undefined, "Champion": 1 });
TS_ASSERT_UNEVAL_EQUALS(cmpEntityLimits.GetCounts(), { "Tower": 0, "Wonder": 0, "Hero": 0, "Champion": 0 });

AddMock(91, IID_TrainingRestrictions, {
	"GetCategory": () => "Hero"
});

cmpEntityLimits.OnGlobalOwnershipChanged({ "entity": 91, "from": -1, "to": 1 });
TS_ASSERT_UNEVAL_EQUALS(cmpEntityLimits.GetLimits(), { "Tower": 5, "Wonder": 1, "Hero": undefined, "Champion": 1 });
TS_ASSERT_UNEVAL_EQUALS(cmpEntityLimits.GetCounts(), { "Tower": 0, "Wonder": 0, "Hero": 1, "Champion": 0 });

cmpEntityLimits.OnGlobalOwnershipChanged({ "entity": 90, "from": 1, "to": -1 });
TS_ASSERT_UNEVAL_EQUALS(cmpEntityLimits.GetLimits(), { "Tower": 5, "Wonder": 1, "Hero": 2, "Champion": 1 });
TS_ASSERT_UNEVAL_EQUALS(cmpEntityLimits.GetCounts(), { "Tower": 0, "Wonder": 0, "Hero": 1, "Champion": 0 });

// Edge case
AddMock(92, IID_TrainingRestrictions, {
	"GetCategory": () => "Hero"
});
AddMock(92, IID_Identity, {
	"GetClassesList": () => ["Aegis"]
});

cmpEntityLimits.OnGlobalOwnershipChanged({ "entity": 92, "from": -1, "to": 1 });
TS_ASSERT_UNEVAL_EQUALS(cmpEntityLimits.GetLimits(), { "Tower": 5, "Wonder": 1, "Hero": undefined, "Champion": 1 });
TS_ASSERT(cmpEntityLimits.AllowedToTrain("Hero", 157));
TS_ASSERT_UNEVAL_EQUALS(cmpEntityLimits.GetCounts(), { "Tower": 0, "Wonder": 0, "Hero": 2, "Champion": 0 });

cmpEntityLimits.OnGlobalOwnershipChanged({ "entity": 91, "from": 1, "to": -1 });
cmpEntityLimits.OnGlobalOwnershipChanged({ "entity": 92, "from": 1, "to": -1 });
TS_ASSERT_UNEVAL_EQUALS(cmpEntityLimits.GetLimits(), { "Tower": 5, "Wonder": 1, "Hero": 2, "Champion": 1 });
TS_ASSERT_UNEVAL_EQUALS(cmpEntityLimits.GetCounts(), { "Tower": 0, "Wonder": 0, "Hero": 0, "Champion": 0 });

// Test AllowedToReplace
AddMock(SYSTEM_ENTITY, IID_TemplateManager, {
	"GetTemplate": name => {
		switch (name)
		{
		case "templateA":
			return { "TrainingRestrictions": { "Category": "Champion" } };
		case "templateB":
			return { "TrainingRestrictions": { "Category": "Hero" } };
		case "templateC":
			return { "BuildRestrictions": { "Category": "Wonder" } };
		case "templateD":
			return { "BuildRestrictions": { "Category": "Tower" } };
		default:
			return null;
		}
	},
	"GetCurrentTemplateName": id => {
		switch (id)
		{
		case 100:
			return "templateA";
		case 101:
			return "templateB";
		case 102:
			return "templateC";
		case 103:
			return "templateD";
		default:
			return null;
		}
	}
});

cmpEntityLimits.ChangeCount( "Champion", 1)
TS_ASSERT(cmpEntityLimits.AllowedToReplace(100, "templateA"))
TS_ASSERT(!cmpEntityLimits.AllowedToReplace(101, "templateA"))
cmpEntityLimits.ChangeCount( "Champion", -1)

cmpEntityLimits.ChangeCount( "Tower", 5)
TS_ASSERT(!cmpEntityLimits.AllowedToReplace(102, "templateD"))
TS_ASSERT(cmpEntityLimits.AllowedToReplace(103, "templateD"))
cmpEntityLimits.ChangeCount( "Tower", -5)

TS_ASSERT_UNEVAL_EQUALS(cmpEntityLimits.GetCounts(), { "Tower": 0, "Wonder": 0, "Hero": 0, "Champion": 0 });

// Test limit removers by tech
cmpEntityLimits.UpdateLimitsFromTech("TechB");
TS_ASSERT_UNEVAL_EQUALS(cmpEntityLimits.GetLimits(), { "Tower": 5, "Wonder": 1, "Hero": 2, "Champion": 1 });

cmpEntityLimits.UpdateLimitsFromTech("TechA");
TS_ASSERT_UNEVAL_EQUALS(cmpEntityLimits.GetLimits(), { "Tower": undefined, "Wonder": 1, "Hero": 2, "Champion": 1 });

cmpEntityLimits.UpdateLimitsFromTech("TechA");
TS_ASSERT_UNEVAL_EQUALS(cmpEntityLimits.GetLimits(), { "Tower": undefined, "Wonder": 1, "Hero": 2, "Champion": 1 });
