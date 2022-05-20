Engine.LoadComponentScript("interfaces/Timer.js");
Engine.LoadComponentScript("interfaces/Formation.js");
Engine.LoadComponentScript("Formation.js");

const entity_id = 5;

AddMock(SYSTEM_ENTITY, IID_Timer, {
	"SetInterval": () => {},
	"SetTimeout": () => {},
});

const formationTemplate = {
	"RequiredMemberCount": 2,
	"DisabledTooltip": "",
	"SpeedMultiplier": 1,
	"FormationShape": "square",
	"MaxTurningAngle": 0,
	"SortingClasses": "Hero Champion Cavalry Melee Ranged",
	"SortingOrder": "fillToTheCenter",
	"ShiftRows": false,
	"UnitSeparationWidthMultiplier": 1,
	"UnitSeparationDepthMultiplier": 1,
	"WidthDepthRatio": 1,
	"Sloppiness": 0
};

const cmpFormation = ConstructComponent(entity_id, "Formation", formationTemplate);

const testingAngles = [];

for (let i = 0; i < 179; i++)
	testingAngles.push(i * Math.PI / 180);

TS_ASSERT(testingAngles.every(x => !cmpFormation.DoesAngleDifferenceAllowTurning(0, x)));
TS_ASSERT(testingAngles.every(x => !cmpFormation.DoesAngleDifferenceAllowTurning(0, -x)));

cmpFormation.maxTurningAngle = Math.PI;

TS_ASSERT(testingAngles.every(x => cmpFormation.DoesAngleDifferenceAllowTurning(0, x)));
TS_ASSERT(testingAngles.every(x => cmpFormation.DoesAngleDifferenceAllowTurning(0, -x)));
