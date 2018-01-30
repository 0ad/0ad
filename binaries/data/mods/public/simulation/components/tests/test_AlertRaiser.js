Engine.LoadHelperScript("Entity.js");
Engine.LoadComponentScript("interfaces/AlertRaiser.js")
Engine.LoadComponentScript("interfaces/GarrisonHolder.js");
Engine.LoadComponentScript("interfaces/UnitAI.js");
Engine.LoadComponentScript("AlertRaiser.js");

const alertRaiserID = 5;
const unitIDs = [10, 11, 12];
const buildingIDs = [13, 14, 15];

let cmpAlertRaiser = ConstructComponent(alertRaiserID, "AlertRaiser", {
	"MaximumLevel": 0,
	"Range": 50
});

Engine.RegisterGlobal("PlaySound", (name, source) => {
	TS_ASSERT_EQUALS(name, "alert" + cmpAlertRaiser.GetLevel());
	TS_ASSERT_EQUALS(source, alertRaiserID);
});

AddMock(alertRaiserID, IID_Ownership, {
	"GetOwner": () => 1
});

AddMock(SYSTEM_ENTITY, IID_RangeManager, {
	"ExecuteQuery": (ent, value, range, players, iid) => iid === IID_UnitAI ? unitIDs : buildingIDs
});

for (let unitID of unitIDs)
	AddMock(unitID, IID_UnitAI, {
		"ReactsToAlert": (alertLevel) => alertLevel >= 2,
		"ReplaceOrder": () => {},
		"HasWorkOrders": () => true,
		"HasGarrisonOrder": () => true,
		"BackToWork": () => {}
	});

for (let buildingID of buildingIDs)
	AddMock(buildingID, IID_GarrisonHolder, {
		"GetCapacity": () => 10,
		"GetGarrisonedEntitiesCount": () => 0,
		"IsAllowedToGarrison": () => true,
		"GetEntities": () => []
	});

TS_ASSERT_EQUALS(cmpAlertRaiser.GetLevel(), 0);
TS_ASSERT_EQUALS(cmpAlertRaiser.HasRaisedAlert(), false);
TS_ASSERT_EQUALS(cmpAlertRaiser.CanIncreaseLevel(), false);
TS_ASSERT_EQUALS(cmpAlertRaiser.IncreaseAlertLevel(), false);
TS_ASSERT_EQUALS(cmpAlertRaiser.GetLevel(), 0);

cmpAlertRaiser = ConstructComponent(alertRaiserID, "AlertRaiser", {
	"MaximumLevel": 2,
	"Range": 50
});

TS_ASSERT_EQUALS(cmpAlertRaiser.GetLevel(), 0);
TS_ASSERT_EQUALS(cmpAlertRaiser.HasRaisedAlert(), false);
TS_ASSERT_EQUALS(cmpAlertRaiser.CanIncreaseLevel(), true);
TS_ASSERT_EQUALS(cmpAlertRaiser.IncreaseAlertLevel(), true);

TS_ASSERT_EQUALS(cmpAlertRaiser.GetLevel(), 1);
TS_ASSERT_EQUALS(cmpAlertRaiser.HasRaisedAlert(), true);
TS_ASSERT_EQUALS(cmpAlertRaiser.CanIncreaseLevel(), true);
TS_ASSERT_EQUALS(cmpAlertRaiser.IncreaseAlertLevel(), true);

TS_ASSERT_EQUALS(cmpAlertRaiser.GetLevel(), 2);
TS_ASSERT_EQUALS(cmpAlertRaiser.HasRaisedAlert(), true);
TS_ASSERT_EQUALS(cmpAlertRaiser.CanIncreaseLevel(), false);
TS_ASSERT_EQUALS(cmpAlertRaiser.IncreaseAlertLevel(), false);

TS_ASSERT_EQUALS(cmpAlertRaiser.EndOfAlert(), true);
TS_ASSERT_EQUALS(cmpAlertRaiser.GetLevel(), 0);
