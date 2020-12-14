Engine.LoadHelperScript("Player.js");
Engine.LoadComponentScript("interfaces/Foundation.js");
Engine.LoadComponentScript("interfaces/Player.js");
Engine.LoadComponentScript("interfaces/Population.js");
Engine.LoadComponentScript("Population.js");

const player = 1;
const entity = 11;
let entPopBonus = 5;

Engine.RegisterGlobal("ApplyValueModificationsToEntity",
	(valueName, currentValue, entity) => currentValue
);

AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
	"GetPlayerByID": () => player
});

let cmpPopulation = ConstructComponent(entity, "Population", {
	"Bonus": entPopBonus
});

// Test ownership change adds bonus.
let cmpPlayer = AddMock(player, IID_Player, {
	"AddPopulationBonuses": bonus => TS_ASSERT_EQUALS(bonus, entPopBonus)
});
let spy = new Spy(cmpPlayer, "AddPopulationBonuses");
cmpPopulation.OnOwnershipChanged({ "from": INVALID_PLAYER, "to": player });
TS_ASSERT_EQUALS(spy._called, 1);

// Test ownership change removes bonus.
cmpPlayer = AddMock(player, IID_Player, {
	"AddPopulationBonuses": bonus => TS_ASSERT_EQUALS(bonus, -entPopBonus)
});
spy = new Spy(cmpPlayer, "AddPopulationBonuses");
cmpPopulation.OnOwnershipChanged({ "from": player, "to": INVALID_PLAYER });
TS_ASSERT_EQUALS(spy._called, 1);


// Test value modifications.
// Test no change.
Engine.RegisterGlobal("ApplyValueModificationsToEntity",
	(valueName, currentValue, entity) => currentValue
);

cmpPlayer = AddMock(player, IID_Player, {
	"AddPopulationBonuses": () => TS_ASSERT(false)
});
cmpPopulation.OnValueModification({ "component": "bogus" });
cmpPopulation.OnValueModification({ "component": "Population" });

// Test changes.
AddMock(entity, IID_Ownership, {
	"GetOwner": () => player
});
let difference = 3;
Engine.RegisterGlobal("ApplyValueModificationsToEntity",
	(valueName, currentValue, entity) => currentValue + difference
);

cmpPlayer = AddMock(player, IID_Player, {
	"AddPopulationBonuses": bonus => TS_ASSERT_EQUALS(bonus, difference)
});
spy = new Spy(cmpPlayer, "AddPopulationBonuses");

// Foundations don't count yet.
AddMock(entity, IID_Foundation, {});
cmpPopulation.OnValueModification({ "component": "Population" });
TS_ASSERT_EQUALS(spy._called, 0);
DeleteMock(entity, IID_Foundation);

cmpPopulation.OnValueModification({ "component": "Population" });
TS_ASSERT_EQUALS(spy._called, 1);

// Reset to no bonus.
cmpPlayer = AddMock(player, IID_Player, {
	"AddPopulationBonuses": bonus => TS_ASSERT_EQUALS(bonus, -3)
});
difference = 0
Engine.RegisterGlobal("ApplyValueModificationsToEntity",
	(valueName, currentValue, entity) => currentValue + difference
);
spy = new Spy(cmpPlayer, "AddPopulationBonuses");
cmpPopulation.OnValueModification({ "component": "Population" });
TS_ASSERT_EQUALS(spy._called, 1);

// Test negative change.
difference = -2;
Engine.RegisterGlobal("ApplyValueModificationsToEntity",
	(valueName, currentValue, entity) => currentValue + difference
);

cmpPlayer = AddMock(player, IID_Player, {
	"AddPopulationBonuses": bonus => TS_ASSERT_EQUALS(bonus, difference)
});
spy = new Spy(cmpPlayer, "AddPopulationBonuses");

cmpPopulation.OnValueModification({ "component": "Population" });
TS_ASSERT_EQUALS(spy._called, 1);

// Test newly created entities also get affected by modifications.
difference = 3;
Engine.RegisterGlobal("ApplyValueModificationsToEntity",
	(valueName, currentValue, entity) => currentValue + difference
);
cmpPlayer = AddMock(player, IID_Player, {
	"AddPopulationBonuses": bonus => TS_ASSERT_EQUALS(bonus, entPopBonus + difference)
});
spy = new Spy(cmpPlayer, "AddPopulationBonuses");
cmpPopulation.OnOwnershipChanged({ "from": INVALID_PLAYER, "to": player });
TS_ASSERT_EQUALS(spy._called, 1);
