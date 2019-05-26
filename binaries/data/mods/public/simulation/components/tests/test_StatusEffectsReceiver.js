Engine.LoadComponentScript("interfaces/Damage.js");
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
	AddMock(SYSTEM_ENTITY, IID_Damage, {
		"CauseDamage": (data) => { dealtDamage += data.strengths[statusName]; }
	});

	// damage scheduled: 0, 10, 20 sec
	cmpStatusReceiver.InflictEffects({
		[statusName]: {
			"Duration": 20000,
			"Interval": 10000,
			"Damage": 1
		}
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
	AddMock(SYSTEM_ENTITY, IID_Damage, {
		"CauseDamage": (data) => {
			if (data.strengths.Burn) dealtDamage += data.strengths.Burn;
			if (data.strengths.Poison) dealtDamage += data.strengths.Poison;
		}
	});

	// damage scheduled: 0, 1, 2, 10 sec
	cmpStatusReceiver.InflictEffects({
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

function testRemoveEffect()
{
	setup();
	let statusName = "Poison";
	AddMock(SYSTEM_ENTITY, IID_Damage, {
		"CauseDamage": (data) => { dealtDamage += data.strengths[statusName]; }
	});

	// damage scheduled: 0, 10, 20 sec
	cmpStatusReceiver.InflictEffects({
		[statusName]: {
			"Duration": 20000,
			"Interval": 10000,
			"Damage": 1
		}
	});

	cmpTimer.OnUpdate({ "turnLength": 1 });
	TS_ASSERT_EQUALS(dealtDamage, 1); // 1 sec

	cmpStatusReceiver.RemoveEffect(statusName);

	cmpTimer.OnUpdate({ "turnLength": 10 });
	TS_ASSERT_EQUALS(dealtDamage, 1); // 11 sec
}

testRemoveEffect();
