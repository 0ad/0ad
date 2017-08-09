Engine.LoadHelperScript("Player.js");
Engine.LoadHelperScript("ValueModification.js");
Engine.LoadHelperScript("Commands.js");
Engine.LoadComponentScript("interfaces/GarrisonHolder.js");
Engine.LoadComponentScript("interfaces/TechnologyManager.js");
Engine.LoadComponentScript("interfaces/AuraManager.js");
Engine.LoadComponentScript("interfaces/Timer.js");
Engine.LoadComponentScript("interfaces/VisionSharing.js");
Engine.LoadComponentScript("VisionSharing.js");

const ent = 170;
let template = {
	"Bribable": "true"
};

AddMock(SYSTEM_ENTITY, IID_TemplateManager, {
	"GetTemplate": (name) =>  name == "special/spy" ?
	({ "Cost": { "Resources": { "wood": 1000 } },
		"VisionSharing": { "Duration": 15 } })
	: ({})
});

AddMock(ent, IID_GarrisonHolder, {
	"GetEntities": () => []
});

AddMock(ent, IID_Ownership, {
	"GetOwner": () => 1
});

let cmpVisionSharing = ConstructComponent(ent, "VisionSharing", template);

// Add some entities
AddMock(180, IID_Ownership, {
	"GetOwner": () => 2
});
AddMock(181, IID_Ownership, {
	"GetOwner": () => 1
});
AddMock(182, IID_Ownership, {
	"GetOwner": () => 8
});
AddMock(183, IID_Ownership, {
	"GetOwner": () => 2
});

TS_ASSERT_EQUALS(cmpVisionSharing.activated, false);

// Test Activate
cmpVisionSharing.activated = false;
cmpVisionSharing.Activate();
TS_ASSERT_EQUALS(cmpVisionSharing.activated, true);
TS_ASSERT_UNEVAL_EQUALS([...cmpVisionSharing.shared], [1]);

// Test CheckVisionSharings
cmpVisionSharing.activated = true;

cmpVisionSharing.shared = new Set([1]);
AddMock(ent, IID_GarrisonHolder, {
	"GetEntities": () => [181]
});
Engine.PostMessage = function(id, iid, message)
{
TS_ASSERT(false); // One doesn't send message
};
cmpVisionSharing.CheckVisionSharings();
TS_ASSERT_UNEVAL_EQUALS([...cmpVisionSharing.shared], [1]);

cmpVisionSharing.shared = new Set([1, 2, 8]);
AddMock(ent, IID_GarrisonHolder, {
	"GetEntities": () => [180]
});
Engine.PostMessage = function(id, iid, message)
{
	TS_ASSERT_UNEVAL_EQUALS({ "entity": ent, "player": 8, "add": false }, message);
};
cmpVisionSharing.CheckVisionSharings();
TS_ASSERT_UNEVAL_EQUALS([...cmpVisionSharing.shared], [1, 2]);

cmpVisionSharing.shared = new Set([1, 8]);
AddMock(ent, IID_GarrisonHolder, {
	"GetEntities": () => [181, 182, 183]
});
Engine.PostMessage = function(id, iid, message)
{
	TS_ASSERT_UNEVAL_EQUALS({ "entity": ent, "player": 2, "add": true }, message);
};
cmpVisionSharing.CheckVisionSharings();
TS_ASSERT_UNEVAL_EQUALS([...cmpVisionSharing.shared], [1, 8, 2]); // take care of order or sort them

// Test IsBribable
TS_ASSERT(cmpVisionSharing.IsBribable());

// Test RemoveSpy
AddMock(ent, IID_GarrisonHolder, {
	"GetEntities": () => []
});
cmpVisionSharing.spies = new Map([[5, 2], [17, 5]]);
cmpVisionSharing.shared = new Set([1, 2, 5]);
Engine.PostMessage = function(id, iid, message)
{
	TS_ASSERT_UNEVAL_EQUALS({ "entity": ent, "player": 2, "add": false }, message);
};
cmpVisionSharing.RemoveSpy({ "id": 5 });
TS_ASSERT_UNEVAL_EQUALS([...cmpVisionSharing.shared], [1, 5]);
TS_ASSERT_UNEVAL_EQUALS([...cmpVisionSharing.spies], [[17, 5]]);
Engine.PostMessage = function(id, iid, message) {};

// Test AddSpy
cmpVisionSharing.spies = new Map([[5, 2], [17, 5]]);
cmpVisionSharing.shared = new Set([1, 2, 5]);
cmpVisionSharing.spyId = 20;

AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
	"GetPlayerByID": id => 14
});

AddMock(14, IID_TechnologyManager, {
	"CanProduce": entity => false,
	"ApplyModificationsTemplate": (valueName, curValue, template) => curValue
});

TS_ASSERT_UNEVAL_EQUALS([...cmpVisionSharing.shared], [1, 2, 5]);
TS_ASSERT_UNEVAL_EQUALS([...cmpVisionSharing.spies], [[5, 2], [17, 5]]);
TS_ASSERT_EQUALS(cmpVisionSharing.spyId, 20);

AddMock(14, IID_TechnologyManager, {
	"CanProduce": entity => entity == "special/spy",
	"ApplyModificationsTemplate": (valueName, curValue, template) => curValue
});
AddMock(14, IID_Player, {
	"GetSpyCostMultiplier": () => 1,
	"TrySubtractResources": costs => false
});
cmpVisionSharing.AddSpy(4, 25);
TS_ASSERT_UNEVAL_EQUALS([...cmpVisionSharing.shared], [1, 2, 5]);
TS_ASSERT_UNEVAL_EQUALS([...cmpVisionSharing.spies], [[5, 2], [17, 5]]);
TS_ASSERT_EQUALS(cmpVisionSharing.spyId, 20);

AddMock(14, IID_Player, {
	"GetSpyCostMultiplier": () => 1,
	"TrySubtractResources": costs => true
});
AddMock(SYSTEM_ENTITY, IID_Timer, {
	"SetTimeout": (ent, iid, funcname, time, data) => TS_ASSERT_EQUALS(time, 25 * 1000)
});
cmpVisionSharing.AddSpy(4, 25);
TS_ASSERT_UNEVAL_EQUALS([...cmpVisionSharing.shared], [1, 2, 5, 4]);
TS_ASSERT_UNEVAL_EQUALS([...cmpVisionSharing.spies], [[5, 2], [17, 5], [21, 4]]);
TS_ASSERT_EQUALS(cmpVisionSharing.spyId, 21);

cmpVisionSharing.spies = new Map([[5, 2], [17, 5]]);
cmpVisionSharing.shared = new Set([1, 2, 5]);
cmpVisionSharing.spyId = 20;
AddMock(ent, IID_Vision, {
	"GetRange": () => 48
});
AddMock(SYSTEM_ENTITY, IID_Timer, {
	"SetTimeout": (ent, iid, funcname, time, data) => TS_ASSERT_EQUALS(time, 15 * 1000 * 60 / 48)
});
cmpVisionSharing.AddSpy(4);
TS_ASSERT_UNEVAL_EQUALS([...cmpVisionSharing.shared], [1, 2, 5, 4]);
TS_ASSERT_UNEVAL_EQUALS([...cmpVisionSharing.spies], [[5, 2], [17, 5], [21, 4]]);
TS_ASSERT_EQUALS(cmpVisionSharing.spyId, 21);

// Test ShareVisionWith
cmpVisionSharing.activated = false;
cmpVisionSharing.shared = undefined;
TS_ASSERT(cmpVisionSharing.ShareVisionWith(1));
TS_ASSERT(!cmpVisionSharing.ShareVisionWith(2));

cmpVisionSharing.activated = true;
cmpVisionSharing.shared = new Set([1, 2, 8]);
TS_ASSERT(cmpVisionSharing.ShareVisionWith(1));
TS_ASSERT(cmpVisionSharing.ShareVisionWith(2));
TS_ASSERT(!cmpVisionSharing.ShareVisionWith(3));
TS_ASSERT(!cmpVisionSharing.ShareVisionWith(0));
