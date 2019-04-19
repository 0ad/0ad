Engine.LoadComponentScript("UnitMotionFlying.js");
Engine.LoadComponentScript("interfaces/Health.js");
Engine.LoadComponentScript("interfaces/GarrisonHolder.js");

let entity = 1;
let target = 2;

let height = 5;

AddMock(SYSTEM_ENTITY, IID_Pathfinder, {
	GetPassabilityClass: (name) => 1 << 8
});

let cmpUnitMotionFlying = ConstructComponent(entity, "UnitMotionFlying", {
	"MaxSpeed": 1.0,
	"TakeoffSpeed": 0.5,
	"LandingSpeed": 0.5,
	"AccelRate": 0.0005,
	"SlowingRate": 0.001,
	"BrakingRate": 0.0005,
	"TurnRate": 0.1,
	"OvershootTime": 10,
	"FlyingHeight": 100,
	"ClimbRate": 0.1,
	"DiesInWater": false,
	"PassabilityClass": "unrestricted"
});

TS_ASSERT_EQUALS(cmpUnitMotionFlying.GetSpeedRatio(), 0);
TS_ASSERT_EQUALS(cmpUnitMotionFlying.GetRunSpeedMultiplier(), 1);
TS_ASSERT_EQUALS(cmpUnitMotionFlying.GetCurrentSpeed(), 0);
cmpUnitMotionFlying.SetSpeedRatio(2);
TS_ASSERT_EQUALS(cmpUnitMotionFlying.GetSpeedRatio(), 0);
TS_ASSERT_EQUALS(cmpUnitMotionFlying.GetRunSpeedMultiplier(), 1);
TS_ASSERT_EQUALS(cmpUnitMotionFlying.GetCurrentSpeed(), 0);

TS_ASSERT_EQUALS(cmpUnitMotionFlying.GetPassabilityClassName(), "unrestricted");
TS_ASSERT_EQUALS(cmpUnitMotionFlying.GetPassabilityClass(), 1 << 8);

AddMock(entity, IID_Position, {
	"IsInWorld": () => true,
	"GetPosition2D": () => { return { "x": 50, "y": 100 }; },
	"GetPosition": () => { return { "x": 50, "y": height, "z": 100 }; },
	"GetRotation": () => { return { "y": 3.14 }; },
	"SetHeightFixed": (y) => height = y,
	"TurnTo": () => {},
	"SetXZRotation": () => {},
	"MoveTo": () => {}
});

AddMock(target, IID_Position, {
	"IsInWorld": () => true,
	"GetPosition2D": () => { return { "x": 100, "y": 200 }; }
});

TS_ASSERT_EQUALS(cmpUnitMotionFlying.IsInTargetRange(target, 10, 112), true);
TS_ASSERT_EQUALS(cmpUnitMotionFlying.IsInTargetRange(target, 50, 111), false);
TS_ASSERT_EQUALS(cmpUnitMotionFlying.IsInTargetRange(target, 112, 200), false);

AddMock(entity, IID_GarrisonHolder, {
	"AllowGarrisoning": () => {}
});

AddMock(entity, IID_Health, {
});

AddMock(entity, IID_RangeManager, {
	"GetLosCircular": () => true
});

AddMock(entity, IID_Terrain, {
	"GetGroundLevel": () => 4,
	"GetMapSize": () => 20
});

AddMock(entity, IID_WaterManager, {
	"GetWaterLevel": () => 5
});

TS_ASSERT_EQUALS(cmpUnitMotionFlying.GetCurrentSpeed(), 0);
cmpUnitMotionFlying.OnUpdate({ "turnLength": 500 });
TS_ASSERT_EQUALS(cmpUnitMotionFlying.GetCurrentSpeed(), 0);
TS_ASSERT_EQUALS(cmpUnitMotionFlying.GetSpeedRatio(), 0);

TS_ASSERT_EQUALS(cmpUnitMotionFlying.MoveToTargetRange(target, 0, 10), true);
TS_ASSERT_EQUALS(cmpUnitMotionFlying.MoveToPointRange(100, 200, 0, 20), true);

// Take Off
cmpUnitMotionFlying.OnUpdate({ "turnLength": 500 });
TS_ASSERT_EQUALS(cmpUnitMotionFlying.GetCurrentSpeed(), 0.25);
TS_ASSERT_EQUALS(height, 5);
cmpUnitMotionFlying.OnUpdate({ "turnLength": 500 });
TS_ASSERT_EQUALS(cmpUnitMotionFlying.GetCurrentSpeed(), 0.5);
TS_ASSERT_EQUALS(height, 5);
cmpUnitMotionFlying.OnUpdate({ "turnLength": 0 });
TS_ASSERT_EQUALS(cmpUnitMotionFlying.GetCurrentSpeed(), 0.5);
TS_ASSERT_EQUALS(height, 5);
cmpUnitMotionFlying.OnUpdate({ "turnLength": 500 });
TS_ASSERT_EQUALS(cmpUnitMotionFlying.GetCurrentSpeed(), 0.75);
TS_ASSERT_EQUALS(height, 55);
cmpUnitMotionFlying.OnUpdate({ "turnLength": 500 });
TS_ASSERT_EQUALS(cmpUnitMotionFlying.GetCurrentSpeed(), 1);
TS_ASSERT_EQUALS(cmpUnitMotionFlying.GetSpeedRatio(), 1);
TS_ASSERT_EQUALS(height, 105);

// Fly
cmpUnitMotionFlying.OnUpdate({ "turnLength": 100 });
TS_ASSERT_EQUALS(cmpUnitMotionFlying.GetCurrentSpeed(), 1);
TS_ASSERT_EQUALS(height, 105);
cmpUnitMotionFlying.OnUpdate({ "turnLength": 500 });
TS_ASSERT_EQUALS(cmpUnitMotionFlying.GetCurrentSpeed(), 1);
TS_ASSERT_EQUALS(height, 105);
cmpUnitMotionFlying.OnUpdate({ "turnLength": 0 });
TS_ASSERT_EQUALS(cmpUnitMotionFlying.GetCurrentSpeed(), 1);
TS_ASSERT_EQUALS(height, 105);

// Land
cmpUnitMotionFlying.StopMoving();
cmpUnitMotionFlying.OnUpdate({ "turnLength": 0 });
TS_ASSERT_EQUALS(cmpUnitMotionFlying.GetCurrentSpeed(), 1);
TS_ASSERT_EQUALS(height, 105);
cmpUnitMotionFlying.OnUpdate({ "turnLength": 500 });
TS_ASSERT_EQUALS(cmpUnitMotionFlying.GetCurrentSpeed(), 0.5);
TS_ASSERT_EQUALS(height, 5);

// Slide
cmpUnitMotionFlying.OnUpdate({ "turnLength": 500 });
TS_ASSERT_EQUALS(cmpUnitMotionFlying.GetCurrentSpeed(), 0.25);
TS_ASSERT_EQUALS(height, 5);
cmpUnitMotionFlying.OnUpdate({ "turnLength": 0 });
TS_ASSERT_EQUALS(cmpUnitMotionFlying.GetCurrentSpeed(), 0.25);
TS_ASSERT_EQUALS(height, 5);
cmpUnitMotionFlying.OnUpdate({ "turnLength": 500 });
TS_ASSERT_EQUALS(cmpUnitMotionFlying.GetCurrentSpeed(), 0);
TS_ASSERT_EQUALS(height, 5);

// Stay
cmpUnitMotionFlying.OnUpdate({ "turnLength": 300 });
TS_ASSERT_EQUALS(cmpUnitMotionFlying.GetCurrentSpeed(), 0);
TS_ASSERT_EQUALS(height, 5);
cmpUnitMotionFlying.OnUpdate({ "turnLength": 0 });
TS_ASSERT_EQUALS(cmpUnitMotionFlying.GetCurrentSpeed(), 0);
TS_ASSERT_EQUALS(height, 5);
cmpUnitMotionFlying.OnUpdate({ "turnLength": 900 });
TS_ASSERT_EQUALS(cmpUnitMotionFlying.GetCurrentSpeed(), 0);
TS_ASSERT_EQUALS(height, 5);

