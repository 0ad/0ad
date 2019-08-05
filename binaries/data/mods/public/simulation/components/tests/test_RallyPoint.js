Engine.LoadHelperScript("Player.js");
Engine.LoadComponentScript("interfaces/Formation.js");
Engine.LoadComponentScript("interfaces/Health.js");
Engine.LoadComponentScript("RallyPoint.js");

function initialRallyPointTest(test_function)
{
	ResetState();

	let entityID = 123;
	let cmpRallyPoint = ConstructComponent(entityID, "RallyPoint", {});

	TS_ASSERT_UNEVAL_EQUALS(cmpRallyPoint.GetData(), []);
	TS_ASSERT_UNEVAL_EQUALS(cmpRallyPoint.GetPositions(), []);

	cmpRallyPoint.AddPosition(3, 1415);
	TS_ASSERT_UNEVAL_EQUALS(cmpRallyPoint.GetPositions(), [{ "x": 3, "z": 1415 }]);

	cmpRallyPoint.AddPosition(926, 535);
	TS_ASSERT_UNEVAL_EQUALS(cmpRallyPoint.GetPositions(), [{ "x": 3, "z": 1415 }, { "x": 926, "z": 535 }]);

	let targetID = 456;
	let myData = { "command": "write a unit test", "target": targetID };
	cmpRallyPoint.AddData(myData);
	TS_ASSERT_UNEVAL_EQUALS(cmpRallyPoint.GetPositions(), [{ "x": 3, "z": 1415 }, { "x": 926, "z": 535 }]);
	TS_ASSERT_UNEVAL_EQUALS(cmpRallyPoint.GetData(), [myData]);

	let targetID2 = 789;
	let myData2 = { "command": "this time really", "target": targetID2 };
	cmpRallyPoint.AddData(myData2);
	TS_ASSERT_UNEVAL_EQUALS(cmpRallyPoint.GetData(), [myData, myData2]);

	if (test_function(cmpRallyPoint))
	{
		TS_ASSERT_UNEVAL_EQUALS(cmpRallyPoint.GetData(), []);
		TS_ASSERT_UNEVAL_EQUALS(cmpRallyPoint.GetPositions(), []);
	}
	else
	{
		TS_ASSERT_UNEVAL_EQUALS(cmpRallyPoint.GetData(), [myData, myData2]);
		TS_ASSERT_UNEVAL_EQUALS(cmpRallyPoint.GetPositions(), [{ "x": 3, "z": 1415 }, { "x": 926, "z": 535 }]);
	}
}

initialRallyPointTest((cmpRallyPoint) => {});

initialRallyPointTest((cmpRallyPoint) => {
	cmpRallyPoint.Unset();
	return true;
});

initialRallyPointTest((cmpRallyPoint) => {
	cmpRallyPoint.Reset();
	return true;
});

// Construction
initialRallyPointTest((cmpRallyPoint) => {
	cmpRallyPoint.OnOwnershipChanged({ "from": INVALID_PLAYER, "to": 1 });
	return false;
});

// Capturing
initialRallyPointTest((cmpRallyPoint) => {
	cmpRallyPoint.OnOwnershipChanged({ "from": 1, "to": 2 });
	return true;
});

// Destruction
initialRallyPointTest((cmpRallyPoint) => {
	cmpRallyPoint.OnOwnershipChanged({ "from": 2, "to": INVALID_PLAYER });
	return false;
});

// Gaia
initialRallyPointTest((cmpRallyPoint) => {
	cmpRallyPoint.OnOwnershipChanged({ "from": 2, "to": 0 });
	return true;
});
