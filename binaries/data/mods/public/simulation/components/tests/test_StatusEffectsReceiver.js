Engine.LoadComponentScript("interfaces/StatusEffectsReceiver.js");
Engine.LoadComponentScript("interfaces/Timer.js");
Engine.LoadComponentScript("StatusEffectsReceiver.js");
Engine.LoadComponentScript("Timer.js");

var target = 42;
var cmpStatusReceiver;
var cmpTimer;
var dealtDamage;

function setup()
{
	cmpStatusReceiver = ConstructComponent(target, "StatusEffectsReceiver");
	cmpTimer = ConstructComponent(SYSTEM_ENTITY, "Timer");
	dealtDamage = 0;
}

function testInflictEffects()
{
	setup();
	let statusName = "Burn";
	let Attacking = {
		"HandleAttackEffects": (_, attackData) => { dealtDamage += attackData.Damage[statusName]; }
	};
	Engine.RegisterGlobal("Attacking", Attacking);

	// damage scheduled: 0, 10, 20 sec
	cmpStatusReceiver.AddStatus(statusName, {
		"Duration": 20000,
		"Interval": 10000,
		"Damage": 1
	});

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
}

testInflictEffects();

function testMultipleEffects()
{
	setup();
	let Attacking = {
		"HandleAttackEffects": (_, attackData) => {
			if (attackData.Damage.Burn) dealtDamage += attackData.Damage.Burn;
			if (attackData.Damage.Poison) dealtDamage += attackData.Damage.Poison;
		},
	};
	Engine.RegisterGlobal("Attacking", Attacking);

	// damage scheduled: 0, 1, 2, 10 sec
	cmpStatusReceiver.GiveStatus({
		"Burn": {
			"Duration": 20000,
			"Interval": 10000,
			"Damage": 10
		},
		"Poison": {
			"Duration": 3000,
			"Interval": 1000,
			"Damage": 1
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
}

testMultipleEffects();

function testRemoveStatus()
{
	setup();
	let statusName = "Poison";
	let Attacking = {
		"HandleAttackEffects": (_, attackData) => { dealtDamage += attackData.Damage[statusName]; }
	};
	Engine.RegisterGlobal("Attacking", Attacking);

	// damage scheduled: 0, 10, 20 sec
	cmpStatusReceiver.AddStatus(statusName, {
		"Duration": 20000,
		"Interval": 10000,
		"Damage": 1
	});

	cmpTimer.OnUpdate({ "turnLength": 1 });
	TS_ASSERT_EQUALS(dealtDamage, 1); // 1 sec

	cmpStatusReceiver.RemoveStatus(statusName);

	cmpTimer.OnUpdate({ "turnLength": 10 });
	TS_ASSERT_EQUALS(dealtDamage, 1); // 11 sec
}

testRemoveStatus();
