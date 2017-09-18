Engine.LoadHelperScript("DamageBonus.js");
Engine.LoadHelperScript("DamageTypes.js");
Engine.LoadHelperScript("ValueModification.js");
Engine.LoadComponentScript("interfaces/AuraManager.js");
Engine.LoadComponentScript("interfaces/Damage.js");
Engine.LoadComponentScript("interfaces/DeathDamage.js");
Engine.LoadComponentScript("interfaces/TechnologyManager.js");
Engine.LoadComponentScript("DeathDamage.js");

let deadEnt = 60;
let player = 1;

ApplyValueModificationsToEntity = function(value, stat, ent)
{
	if (value == "DeathDamage/Pierce" && ent == deadEnt)
		return stat + 200;
	return stat;
};

let template = {
	"Shape": "Circular",
	"Range": 10.7,
	"FriendlyFire": "false",
	"Hack": 0.0,
	"Pierce": 15.0,
	"Crush": 35.0
};

let modifiedDamage = {
	"Hack": 0.0,
	"Pierce": 215.0,
	"Crush": 35.0
};

let cmpDeathDamage = ConstructComponent(deadEnt, "DeathDamage", template);

let playersToDamage = [2, 3, 7];
let pos = new Vector2D(3, 4.2);

let result = {
	"attacker": deadEnt,
	"origin": pos,
	"radius": template.Range,
	"shape": template.Shape,
	"strengths": modifiedDamage,
	"splashBonus": null,
	"playersToDamage": playersToDamage,
	"type": "Death",
	"attackerOwner": player
};

AddMock(SYSTEM_ENTITY, IID_Damage, {
	"CauseSplashDamage": data => TS_ASSERT_UNEVAL_EQUALS(data, result),
	"GetPlayersToDamage": (owner, friendlyFire) => playersToDamage
});

AddMock(deadEnt, IID_Position, {
	"GetPosition2D": () => pos,
	"IsInWorld": () => true
});

AddMock(deadEnt, IID_Ownership, {
	"GetOwner": () => player
});

TS_ASSERT_UNEVAL_EQUALS(cmpDeathDamage.GetDeathDamageStrengths(), modifiedDamage);
cmpDeathDamage.CauseDeathDamage();

// Test splash damage bonus
let splashBonus = { "BonusCav": { "Classes": "Cavalry", "Multiplier": 3 } };
template.Bonuses = splashBonus;
cmpDeathDamage = ConstructComponent(deadEnt, "DeathDamage", template);
result.splashBonus = splashBonus;
TS_ASSERT_UNEVAL_EQUALS(cmpDeathDamage.GetDeathDamageStrengths(), modifiedDamage);
cmpDeathDamage.CauseDeathDamage();
