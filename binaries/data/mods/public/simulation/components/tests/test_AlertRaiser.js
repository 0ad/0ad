Engine.LoadComponentScript("interfaces/AlertRaiser.js")
Engine.LoadComponentScript("interfaces/ProductionQueue.js");
Engine.LoadComponentScript("interfaces/Sound.js");
Engine.LoadComponentScript("interfaces/UnitAI.js");
Engine.LoadComponentScript("AlertRaiser.js");

const alertRaiserId = 5;
const unitsIds = [10, 11, 12];
const buildingsIds = [13, 14, 15];

let cmpAlertRaiser = ConstructComponent(alertRaiserId, "AlertRaiser", {
	"MaximumLevel": 0,
	"Range": 50
});

Engine.RegisterGlobal("PlaySound", (name, source) => {
	TS_ASSERT_EQUALS(name, "alert" + cmpAlertRaiser.GetLevel());
	TS_ASSERT_EQUALS(source, alertRaiserId);
});

AddMock(SYSTEM_ENTITY, IID_RangeManager, {
	"ExecuteQuery": (ent, value, range, players, iid) => iid === IID_UnitAI ? unitsIds : buildingsIds
});

unitsIds.forEach((unitId) => {
	AddMock(unitId, IID_UnitAI, {
		"ReactsToAlert": (alertLevel) => alertLevel >= 2,
		"ReplaceOrder": () => {},
		"IsUnderAlert": () => {},
		"HasWorkOrders": () => true,
		"BackToWork": () => {},
		"ResetAlert": () => {}
	});
});

buildingsIds.forEach((buildingId) => {
	AddMock(buildingId, IID_ProductionQueue, {
		"PutUnderAlert": (alertRaiserId) => {},
		"ResetAlert": () => {}
	});
});

TS_ASSERT_EQUALS(cmpAlertRaiser.GetLevel(), 0);
TS_ASSERT_EQUALS(cmpAlertRaiser.HasRaisedAlert(), false);
TS_ASSERT_EQUALS(cmpAlertRaiser.CanIncreaseLevel(), false);
TS_ASSERT_EQUALS(cmpAlertRaiser.IncreaseAlertLevel(), false);

cmpAlertRaiser = ConstructComponent(alertRaiserId, "AlertRaiser", {
	"MaximumLevel": 2,
	"Range": 50
});

TS_ASSERT_EQUALS(cmpAlertRaiser.CanIncreaseLevel(), true);

cmpAlertRaiser.UpdateUnits([]);
cmpAlertRaiser.UpdateUnits(unitsIds);

TS_ASSERT_UNEVAL_EQUALS(cmpAlertRaiser.walkingUnits, []);
TS_ASSERT_EQUALS(cmpAlertRaiser.IncreaseAlertLevel(), true);
TS_ASSERT_EQUALS(cmpAlertRaiser.GetLevel(), 1);
TS_ASSERT_UNEVAL_EQUALS(cmpAlertRaiser.prodBuildings, buildingsIds);
TS_ASSERT_EQUALS(cmpAlertRaiser.IncreaseAlertLevel(), true);
TS_ASSERT_EQUALS(cmpAlertRaiser.GetLevel(), 2);
TS_ASSERT_UNEVAL_EQUALS(cmpAlertRaiser.walkingUnits, unitsIds);
TS_ASSERT_EQUALS(cmpAlertRaiser.IncreaseAlertLevel(), false);
TS_ASSERT_EQUALS(cmpAlertRaiser.HasRaisedAlert(), true);
TS_ASSERT_EQUALS(cmpAlertRaiser.EndOfAlert(), true);
TS_ASSERT_EQUALS(cmpAlertRaiser.GetLevel(), 0);
