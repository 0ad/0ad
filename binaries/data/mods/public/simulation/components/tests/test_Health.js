Engine.LoadComponentScript("interfaces/TechnologyManager.js");
Engine.LoadComponentScript("interfaces/AuraManager.js");
Engine.LoadHelperScript("Player.js");
Engine.LoadHelperScript("ValueModification.js");

Engine.LoadHelperScript("Sound.js");
Engine.LoadComponentScript("interfaces/DeathDamage.js");

Engine.LoadComponentScript("interfaces/Health.js");
Engine.LoadComponentScript("Health.js");

const entity_id = 5;
const corpse_id = entity_id + 1;

const health_template = {
	"Max": 50,
	"RegenRate": 0,
	"IdleRegenRate": 0,
	"DeathType": "corpse",
	"Unhealable": false
};


var injured_flag = false;
var corpse_entity;

function setEntityUp()
{
	let cmpHealth = ConstructComponent(entity_id, "Health", health_template);

	AddMock(entity_id, IID_DeathDamage, {
		"CauseDeathDamage": () => {}
	});
	AddMock(entity_id, IID_Position, {
		"IsInWorld": () => true,
		"GetPosition": () => ({ "x": 0, "z": 0 }),
		"GetRotation": () => ({ "x": 0, "y": 0, "z": 0 })
	});
	AddMock(entity_id, IID_Ownership, {
		"GetOwner": () => 1
	});
	AddMock(entity_id, IID_Visual, {
		"GetActorSeed": () => 1
	});
	AddMock(SYSTEM_ENTITY, IID_TemplateManager, {
		"GetCurrentTemplateName": () => "test"
	});

	AddMock(SYSTEM_ENTITY, IID_RangeManager, {
		"SetEntityFlag": (ent, flag, value) => (injured_flag = value)
	});

	return cmpHealth;
}

var cmpHealth = setEntityUp();

TS_ASSERT_EQUALS(cmpHealth.GetHitpoints(), 50);
TS_ASSERT_EQUALS(cmpHealth.GetMaxHitpoints(), 50);
TS_ASSERT_EQUALS(cmpHealth.IsInjured(), false);
TS_ASSERT_EQUALS(cmpHealth.IsUnhealable(), true);

var change = cmpHealth.Reduce(25);
TS_ASSERT_EQUALS(injured_flag, true);

TS_ASSERT_EQUALS(change.killed, false);
TS_ASSERT_EQUALS(change.HPchange, -25);
TS_ASSERT_EQUALS(cmpHealth.GetHitpoints(), 25);
TS_ASSERT_EQUALS(cmpHealth.GetMaxHitpoints(), 50);
TS_ASSERT_EQUALS(cmpHealth.IsInjured(), true);
TS_ASSERT_EQUALS(cmpHealth.IsUnhealable(), false);

change = cmpHealth.Increase(25);
TS_ASSERT_EQUALS(injured_flag, false);

TS_ASSERT_EQUALS(change.new, 50);
TS_ASSERT_EQUALS(cmpHealth.GetHitpoints(), 50);
TS_ASSERT_EQUALS(cmpHealth.GetMaxHitpoints(), 50);
TS_ASSERT_EQUALS(cmpHealth.IsInjured(), false);
TS_ASSERT_EQUALS(cmpHealth.IsUnhealable(), true);

// Check death.
Engine.AddLocalEntity = function(template) {
	corpse_entity = template;

	AddMock(corpse_id, IID_Position, {
		"JumpTo": () => {},
		"SetYRotation": () => {},
		"SetXZRotation": () => {},
	});
	AddMock(corpse_id, IID_Ownership, {
		"SetOwner": () => {},
	});
	AddMock(corpse_id, IID_Visual, {
		"SetActorSeed": () => {},
		"SelectAnimation": () => {},
	});
	return corpse_id;
};

change = cmpHealth.Reduce(50);

// Assert we create a corpse with the proper template.
TS_ASSERT_EQUALS(corpse_entity, "corpse|test");

// Check that we are not marked as injured.
TS_ASSERT_EQUALS(injured_flag, false);

TS_ASSERT_EQUALS(change.killed, true);
TS_ASSERT_EQUALS(change.HPchange, -50);
TS_ASSERT_EQUALS(cmpHealth.GetHitpoints(), 0);
TS_ASSERT_EQUALS(cmpHealth.GetMaxHitpoints(), 50);
TS_ASSERT_EQUALS(cmpHealth.IsInjured(), false);

// Check that we can't be revived once dead.
change = cmpHealth.Increase(25);
TS_ASSERT_EQUALS(change.new, 0);
TS_ASSERT_EQUALS(cmpHealth.GetHitpoints(), 0);
TS_ASSERT_EQUALS(cmpHealth.GetMaxHitpoints(), 50);
TS_ASSERT_EQUALS(cmpHealth.IsInjured(), false);

// Check that we can't die twice.
change = cmpHealth.Reduce(50);
TS_ASSERT_EQUALS(change.killed, false);
TS_ASSERT_EQUALS(change.HPchange, 0);
TS_ASSERT_EQUALS(cmpHealth.GetHitpoints(), 0);
TS_ASSERT_EQUALS(cmpHealth.GetMaxHitpoints(), 50);
TS_ASSERT_EQUALS(cmpHealth.IsInjured(), false);

cmpHealth = setEntityUp();

// Check that we still die with > Max HP of damage.
change = cmpHealth.Reduce(60);
TS_ASSERT_EQUALS(change.killed, true);
TS_ASSERT_EQUALS(change.HPchange, -50);
TS_ASSERT_EQUALS(cmpHealth.GetHitpoints(), 0);
TS_ASSERT_EQUALS(cmpHealth.GetMaxHitpoints(), 50);
TS_ASSERT_EQUALS(cmpHealth.IsInjured(), false);

cmpHealth = setEntityUp();

// Check that increasing by more than required puts us at the max HP
change = cmpHealth.Reduce(30);
change = cmpHealth.Increase(30);
TS_ASSERT_EQUALS(injured_flag, false);
TS_ASSERT_EQUALS(change.new, 50);
TS_ASSERT_EQUALS(cmpHealth.GetHitpoints(), 50);
TS_ASSERT_EQUALS(cmpHealth.GetMaxHitpoints(), 50);
TS_ASSERT_EQUALS(cmpHealth.IsInjured(), false);
TS_ASSERT_EQUALS(cmpHealth.IsUnhealable(), true);
