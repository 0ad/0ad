Engine.LoadComponentScript("interfaces/Gate.js");
Engine.LoadComponentScript("interfaces/UnitAI.js");
Engine.LoadComponentScript("Gate.js");

function testBasicBehaviour()
{
	const gate = 10;
	const own = 11;
	const passRange = 20;

	Engine.RegisterGlobal("QueryPlayerIDInterface", () => ({
		"GetAllies": () => [1, 2],
	}));
	Engine.RegisterGlobal("PlaySound", () => {});

	let cmpRangeMgr = AddMock(SYSTEM_ENTITY, IID_RangeManager, {
		"GetEntityFlagMask": () => {},
		"CreateActiveQuery": () => {},
		"EnableActiveQuery": () => {},
	});
	let querySpy = new Spy(cmpRangeMgr, "CreateActiveQuery");

	let ownUnitAI = AddMock(own, IID_UnitAI, {
		"AbleToMove": () => true
	});

	let cmpGate = ConstructComponent(gate, "Gate", {
		"PassRange": passRange
	});
	let setupSpy = new Spy(cmpGate, "SetupRangeQuery");
	let cmpGateObst = AddMock(gate, IID_Obstruction, {
		"SetDisableBlockMovementPathfinding": () => {},
		"GetEntitiesBlockingConstruction": () => [],
		"GetBlockMovementFlag": () => false,
	});
	AddMock(gate, IID_Ownership, {
		"GetOwner": () => 1,
	});

	// Test that gates are closed at startup.
	TS_ASSERT_EQUALS(cmpGate.locked, false);
	cmpGate.OnOwnershipChanged({ "from": INVALID_PLAYER, "to": 1 });
	TS_ASSERT_EQUALS(setupSpy._called, 1);
	TS_ASSERT_EQUALS(querySpy._callargs[0][2], passRange);
	TS_ASSERT_UNEVAL_EQUALS(querySpy._callargs[0][3], [1, 2]);
	TS_ASSERT_EQUALS(cmpGate.opened, false);

	// Test that they open if units get in range
	cmpGate.OnRangeUpdate({ "tag": cmpGate.unitsQuery, "added": [own], "removed": [] });
	TS_ASSERT_EQUALS(cmpGate.opened, true);
	TS_ASSERT_UNEVAL_EQUALS(cmpGate.allies, [own]);
	TS_ASSERT_UNEVAL_EQUALS(cmpGate.ignoreList, []);

	// Assert that it closes if the unit says it can't move anymore.
	cmpGate.OnGlobalUnitAbleToMoveChanged({ "entity": own });
	TS_ASSERT_EQUALS(cmpGate.opened, false);
	TS_ASSERT_UNEVAL_EQUALS(cmpGate.ignoreList, [own]);

	// Assert that it is OK if the entity goes away
	cmpGate.OnRangeUpdate({ "tag": cmpGate.unitsQuery, "added": [], "removed": [own] });
	TS_ASSERT_EQUALS(cmpGate.opened, false);
	TS_ASSERT_UNEVAL_EQUALS(cmpGate.allies, []);
	TS_ASSERT_UNEVAL_EQUALS(cmpGate.ignoreList, []);

	// Lock the gates, try again.
	cmpGate.LockGate();
	TS_ASSERT(cmpGate.IsLocked());
	cmpGate.OnRangeUpdate({ "tag": cmpGate.unitsQuery, "added": [own], "removed": [] });
	TS_ASSERT_EQUALS(cmpGate.opened, false);
	TS_ASSERT_UNEVAL_EQUALS(cmpGate.allies, [own]);

	cmpGate.UnlockGate();
	TS_ASSERT_EQUALS(cmpGate.opened, true);
	cmpGate.LockGate();
	TS_ASSERT_EQUALS(cmpGate.opened, false);

	// Finally, trigger some other handlers to see if things remain correct.
	setupSpy._reset();
	cmpGate.OnOwnershipChanged({ "from": 1, "to": 2 });
	TS_ASSERT_EQUALS(setupSpy._called, 1);
	cmpGate.OnDiplomacyChanged({ "player": 1 });
	TS_ASSERT_EQUALS(setupSpy._called, 2);
}

function testShouldOpen()
{
	let cmpGate = ConstructComponent(5, "Gate", {});
	cmpGate.allies = [1, 2, 3, 4];
	cmpGate.ignoreList = [];
	TS_ASSERT_EQUALS(cmpGate.ShouldOpen(), true);
	cmpGate.ignoreList = [2, 3];
	TS_ASSERT_EQUALS(cmpGate.ShouldOpen(), true);
	cmpGate.ignoreList = [1, 2, 3, 4];
	TS_ASSERT_EQUALS(cmpGate.ShouldOpen(), false);
	cmpGate.allies = [];
	cmpGate.ignoreList = [];
	TS_ASSERT_EQUALS(cmpGate.ShouldOpen(), false);
}

testBasicBehaviour();
testShouldOpen();
