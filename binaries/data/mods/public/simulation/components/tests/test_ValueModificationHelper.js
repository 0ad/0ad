Engine.LoadHelperScript("Player.js");
Engine.LoadHelperScript("ValueModification.js");
Engine.LoadComponentScript("interfaces/AuraManager.js");
Engine.LoadComponentScript("interfaces/Player.js");
Engine.LoadComponentScript("interfaces/TechnologyManager.js");

let player = 1;
let playerEnt = 10;
let ownedEnt = 60;
let techKey = "Attack/BigAttack";

AddMock(playerEnt, IID_TechnologyManager, {
	"ApplyModifications": (key, val, ent) => {
		if (key != techKey)
			return val;
		if (ent == playerEnt)
			return val + 3;
		if (ent == ownedEnt)
			return val + 7;
		return val;
	}
});

AddMock(SYSTEM_ENTITY, IID_AuraManager, {
	"ApplyModifications": (key, val, ent) => {
		if (key != techKey)
			return val;
		if (ent == playerEnt)
			return val * 10;
		if (ent == ownedEnt)
			return val * 100;
		return val;
	}
});

AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
	"GetPlayerByID": () => 10
});

AddMock(playerEnt, IID_Player, {
	"GetPlayerID": () => 1
});

AddMock(ownedEnt, IID_Ownership, {
	"GetOwner": () => 1
});

TS_ASSERT_EQUALS(ApplyValueModificationsToEntity(techKey, 2.0, playerEnt), 50.0);

TS_ASSERT_EQUALS(ApplyValueModificationsToEntity(techKey, 2.0, ownedEnt), 900.0);
