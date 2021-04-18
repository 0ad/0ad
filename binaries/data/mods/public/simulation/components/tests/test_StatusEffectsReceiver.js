Engine.LoadHelperScript("Player.js");
Engine.LoadHelperScript("ValueModification.js");
Engine.LoadComponentScript("interfaces/Health.js");
Engine.LoadComponentScript("interfaces/ModifiersManager.js");
Engine.LoadComponentScript("interfaces/StatusEffectsReceiver.js");
Engine.LoadComponentScript("interfaces/Timer.js");
Engine.LoadComponentScript("Health.js");
Engine.LoadComponentScript("ModifiersManager.js");
Engine.LoadComponentScript("StatusEffectsReceiver.js");
Engine.LoadComponentScript("Timer.js");

let target = 42;
let cmpStatusReceiver = ConstructComponent(target, "StatusEffectsReceiver");
let cmpTimer = ConstructComponent(SYSTEM_ENTITY, "Timer");
let dealtDamage = 0;
let enemyEntity = 4;
let enemy = 2;
let statusName;

let AttackHelper = {
	"HandleAttackEffects": (_, data) => {
		for (let type in data.attackData.Damage)
			dealtDamage += data.attackData.Damage[type];
	}
};
Engine.RegisterGlobal("AttackHelper", AttackHelper);

function reset()
{
	for (let status in cmpStatusReceiver.GetActiveStatuses())
		cmpStatusReceiver.RemoveStatus(status);
	dealtDamage = 0;
}

// Test adding a single effect.
statusName = "Burn";

// Damage scheduled: 0, 10, 20 seconds.
cmpStatusReceiver.AddStatus(statusName, {
	"Duration": 20000,
	"Interval": 10000,
	"Damage": {
		[statusName]: 1
	}
},
enemyEntity,
enemy
);

cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(dealtDamage, 1); // 1 sec

cmpTimer.OnUpdate({ "turnLength": 8 });
TS_ASSERT_EQUALS(dealtDamage, 1); // 9 sec

cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(dealtDamage, 2); // 10 sec

cmpTimer.OnUpdate({ "turnLength": 10 });
TS_ASSERT_EQUALS(dealtDamage, 3); // 20 sec

cmpTimer.OnUpdate({ "turnLength": 10 });
TS_ASSERT_EQUALS(dealtDamage, 3); // 30 sec


// Test adding multiple effects.
reset();

// Damage scheduled: 0, 1, 2, 10 seconds.
cmpStatusReceiver.ApplyStatus({
	"Burn": {
		"Duration": 20000,
		"Interval": 10000,
		"Damage": {
			"Burn": 10
		}
	},
	"Poison": {
		"Duration": 3000,
		"Interval": 1000,
		"Damage": {
			"Poison": 1
		}
	}
});

cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(dealtDamage, 12); // 1 sec

cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(dealtDamage, 13); // 2 sec

cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(dealtDamage, 13); // 3 sec

cmpTimer.OnUpdate({ "turnLength": 7 });
TS_ASSERT_EQUALS(dealtDamage, 23); // 10 sec


// Test removing a status removes effects.
reset();
statusName = "Poison";

// Damage scheduled: 0, 10, 20 seconds.
cmpStatusReceiver.AddStatus(statusName, {
	"Duration": 20000,
	"Interval": 10000,
	"Damage": {
		[statusName]: 1
	}
},
enemyEntity,
enemy
);

cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(dealtDamage, 1); // 1 sec

cmpStatusReceiver.RemoveStatus(statusName);

cmpTimer.OnUpdate({ "turnLength": 10 });
TS_ASSERT_EQUALS(dealtDamage, 1); // 11 sec


// Test that a status effect with modifications modifies.
reset();

AddMock(target, IID_Identity, {
	"GetClassesList": () => ["AffectedClass"]
});
let cmpModifiersManager = ConstructComponent(SYSTEM_ENTITY, "ModifiersManager");

let maxHealth = 100;
AddMock(target, IID_Health, {
	"GetMaxHitpoints": () => ApplyValueModificationsToEntity("Health/Max", maxHealth, target)
});

statusName = "Haste";
let factor = 0.5;
cmpStatusReceiver.AddStatus(statusName, {
	"Duration": 5000,
	"Modifiers": {
		[statusName]: {
			"Paths": {
				"_string": "Health/Max"
			},
			"Affects": {
				"_string": "AffectedClass"
			},
			"Multiply": factor
		}
	}
},
enemyEntity,
enemy
);

let cmpHealth = Engine.QueryInterface(target, IID_Health);
// Test that the modification is applied.
TS_ASSERT_EQUALS(cmpHealth.GetMaxHitpoints(), maxHealth * factor);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpHealth.GetMaxHitpoints(), maxHealth * factor);

// Test that the modification is removed after the appropriate time.
cmpTimer.OnUpdate({ "turnLength": 4 });
TS_ASSERT_EQUALS(cmpHealth.GetMaxHitpoints(), maxHealth);


// Test addition.
let addition = 50;
cmpStatusReceiver.AddStatus(statusName, {
	"Duration": 5000,
	"Modifiers": {
		[statusName]: {
			"Paths": {
				"_string": "Health/Max"
			},
			"Affects": {
				"_string": "AffectedClass"
			},
			"Add": addition
		}
	}
},
enemyEntity,
enemy
);

// Test that the addition modification is applied.
TS_ASSERT_EQUALS(cmpHealth.GetMaxHitpoints(), maxHealth + addition);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpHealth.GetMaxHitpoints(), maxHealth + addition);

// Test that the modification is removed after the appropriate time.
cmpTimer.OnUpdate({ "turnLength": 4 });
TS_ASSERT_EQUALS(cmpHealth.GetMaxHitpoints(), maxHealth);


// Test replacement.
let newValue = 50;
cmpStatusReceiver.AddStatus(statusName, {
	"Duration": 5000,
	"Modifiers": {
		[statusName]: {
			"Paths": {
				"_string": "Health/Max"
			},
			"Affects": {
				"_string": "AffectedClass"
			},
			"Replace": newValue
		}
	}
},
enemyEntity,
enemy
);

// Test that the replacement modification is applied.
TS_ASSERT_EQUALS(cmpHealth.GetMaxHitpoints(), newValue);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpHealth.GetMaxHitpoints(), newValue);

// Test that the modification is removed after the appropriate time.
cmpTimer.OnUpdate({ "turnLength": 4 });
TS_ASSERT_EQUALS(cmpHealth.GetMaxHitpoints(), maxHealth);


function applyStatus(stackability)
{
	cmpStatusReceiver.ApplyStatus({
		"randomName": {
			"Duration": 3000,
			"Interval": 1000,
			"Damage": {
				"randomName": 1
			},
			"Stackability": stackability
		}
	});
}


// Test different stackabilities.
// First ignoring, i.e. next time the same status is added it is just ignored.
reset();
applyStatus("Ignore");

// 1 Second: 1 update and lateness.
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(dealtDamage, 2);

applyStatus("Ignore");

// 2 Seconds.
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(dealtDamage, 3);

// 3 Seconds: finished in previous turn.
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(dealtDamage, 3);


// Extending, i.e. next time the same status is applied the times are added.
reset();
applyStatus("Extend");

// 1 Second: 1 update and lateness.
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(dealtDamage, 2);

// Add 3 seconds.
applyStatus("Extend");

// 2 Seconds.
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(dealtDamage, 3);

// 3 Seconds: extended in previous turn.
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(dealtDamage, 4);

// 4 Seconds.
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(dealtDamage, 5);

// 5 Seconds.
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(dealtDamage, 6);

// 6 Seconds: finished in previous turn (3 + 3).
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(dealtDamage, 6);


// Replacing, i.e. the next applied status replaces the former.
reset();
applyStatus("Replace");

// 1 Second: 1 update and lateness.
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(dealtDamage, 2);

applyStatus("Replace");

// 2 Seconds: 1 update and lateness of the new status.
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(dealtDamage, 4);

// 3 Seconds.
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(dealtDamage, 5);

// 4 Seconds: finished in previous turn.
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(dealtDamage, 5);


// Stacking, every new status just applies besides the rest.
reset();
applyStatus("Stack");

// 1 Second: 1 update and lateness.
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(dealtDamage, 2);

applyStatus("Stack");

// 2 Seconds: 1 damage from the previous status + 2 from the new (1 turn + lateness).
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(dealtDamage, 5);

// 3 Seconds: first one finished in the previous turn, +1 from the new.
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(dealtDamage, 6);

// 4 Seconds: new status finished in previous turn.
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(dealtDamage, 6);
