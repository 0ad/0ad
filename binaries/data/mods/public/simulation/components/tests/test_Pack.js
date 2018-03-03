Engine.LoadHelperScript("Player.js");
Engine.LoadHelperScript("Sound.js");
Engine.LoadHelperScript("Transform.js");
Engine.LoadHelperScript("ValueModification.js");
Engine.LoadComponentScript("interfaces/TechnologyManager.js");
Engine.LoadComponentScript("interfaces/AuraManager.js");
Engine.LoadComponentScript("interfaces/Capturable.js");
Engine.LoadComponentScript("interfaces/GarrisonHolder.js");
Engine.LoadComponentScript("interfaces/Guard.js");
Engine.LoadComponentScript("interfaces/Health.js");
Engine.LoadComponentScript("interfaces/Pack.js");
Engine.LoadComponentScript("interfaces/Player.js");
Engine.LoadComponentScript("interfaces/Timer.js");
Engine.LoadComponentScript("interfaces/UnitAI.js");
Engine.LoadComponentScript("Pack.js");
Engine.RegisterGlobal("MT_EntityRenamed", "entityRenamed");

const ent = 170;
const newEnt = 171;
const PACKING_INTERVAL = 250;
let timerActivated = false;

AddMock(ent, IID_Visual, {
	"SelectAnimation": (name, once, speed) => name
});

AddMock(ent, IID_Ownership, {
	"GetOwner": () => 1
});

AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
	"GetPlayerByID": id => 11
});

AddMock(11, IID_Player, {
	"GetTimeMultiplier": () => 1
});

AddMock(ent, IID_Sound, {
	"PlaySoundGroup": name => {}
});

AddMock(SYSTEM_ENTITY, IID_Timer, {
	"CancelTimer": id => { timerActivated = false; return; },
	"SetInterval": (ent, iid, funcname, time, repeattime, data) => { timerActivated = true; return 7; }
});

Engine.AddEntity = function(template) {
	TS_ASSERT_EQUALS(template, "finalTemplate");
	return true;
};

// Test Packing

let template = {
	"Entity": "finalTemplate",
	"Time": "2000",
	"State": "unpacked"
};
let cmpPack = ConstructComponent(ent, "Pack", template);

// Check internals
TS_ASSERT(!cmpPack.packed);
TS_ASSERT(!cmpPack.packing);
TS_ASSERT_EQUALS(cmpPack.elapsedTime, 0);
TS_ASSERT_EQUALS(cmpPack.timer, undefined);

TS_ASSERT(!cmpPack.IsPacked());
TS_ASSERT(!cmpPack.IsPacking());
TS_ASSERT_EQUALS(cmpPack.GetProgress(), 0);

// Pack
cmpPack.Pack();
TS_ASSERT_EQUALS(cmpPack.timer, 7);
TS_ASSERT(cmpPack.IsPacking());

// Listen to destroy message
cmpPack.OnDestroy();
TS_ASSERT(!cmpPack.timer);
TS_ASSERT(!timerActivated);

// Test UnPacking

template = {
	"Entity": "finalTemplate",
	"Time": "2000",
	"State": "packed"
};

cmpPack = ConstructComponent(ent, "Pack", template);

// Check internals
TS_ASSERT(cmpPack.packed);
TS_ASSERT(!cmpPack.packing);
TS_ASSERT_EQUALS(cmpPack.elapsedTime, 0);
TS_ASSERT_EQUALS(cmpPack.timer, undefined);

TS_ASSERT(cmpPack.IsPacked());
TS_ASSERT(!cmpPack.IsPacking());
TS_ASSERT_EQUALS(cmpPack.GetProgress(), 0);

// Unpack
cmpPack.Unpack();
TS_ASSERT(cmpPack.IsPacking());
TS_ASSERT_EQUALS(cmpPack.timer, 7);

// Unpack progress
cmpPack.elapsedTime = 400;
cmpPack.PackProgress({}, 100);

TS_ASSERT(cmpPack.IsPacking());
TS_ASSERT_EQUALS(cmpPack.GetElapsedTime(), 400 + 100 + PACKING_INTERVAL);
TS_ASSERT_EQUALS(cmpPack.GetProgress(), (400 + 100 + PACKING_INTERVAL) / 2000);

// Try to Pack or Unpack while packing, nothing happen
cmpPack.elapsedTime = 400;

cmpPack.Unpack();
TS_ASSERT(cmpPack.IsPacking());
TS_ASSERT_EQUALS(cmpPack.GetElapsedTime(), 400);
TS_ASSERT_EQUALS(cmpPack.timer, 7);
TS_ASSERT(timerActivated);

cmpPack.Pack();
TS_ASSERT(cmpPack.IsPacking());
TS_ASSERT_EQUALS(cmpPack.GetElapsedTime(), 400);
TS_ASSERT_EQUALS(cmpPack.timer, 7);
TS_ASSERT(timerActivated);

// Cancel
cmpPack.CancelPack();

TS_ASSERT(!cmpPack.IsPacking());
TS_ASSERT_EQUALS(cmpPack.GetElapsedTime(), 0);
TS_ASSERT_EQUALS(cmpPack.GetProgress(), 0);
TS_ASSERT_EQUALS(cmpPack.timer, undefined);
TS_ASSERT(!timerActivated);

// Progress until completing
cmpPack.Unpack();
cmpPack.elapsedTime = 1800;
cmpPack.PackProgress({}, 100);

TS_ASSERT(cmpPack.IsPacking());
TS_ASSERT_EQUALS(cmpPack.GetElapsedTime(), 1800 + 100 + PACKING_INTERVAL);
// Cap progress at 100%
TS_ASSERT_EQUALS(cmpPack.GetProgress(), 1);
TS_ASSERT_EQUALS(cmpPack.timer, 7);
TS_ASSERT(timerActivated);

// Unpack completing
cmpPack.Unpack();
cmpPack.elapsedTime = 2100;
cmpPack.PackProgress({}, 100);

TS_ASSERT(!cmpPack.IsPacking());
TS_ASSERT(!cmpPack.timer);
TS_ASSERT(!timerActivated);
