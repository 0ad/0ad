Resources = {
	"BuildChoicesSchema": () => {
		let schema = "";
		for (let res of ["food", "metal"])
		{
			for (let subtype in ["meat", "grain"])
				schema += "<value>" + res + "." + subtype + "</value>";
			schema += "<value> treasure." + res + "</value>";
		}
		return "<choice>" + schema + "</choice>";
	}
};

Engine.LoadComponentScript("interfaces/ResourceSupply.js");
Engine.LoadComponentScript("ResourceSupply.js");

const entity = 60;

AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
	"GetNumPlayers": () => 2
});

AddMock(entity, IID_Fogging, {
	"Activate": () => {}
});

let template = {
	"Amount": 1000,
	"Type": "food.meat",
	"KillBeforeGather": false,
	"MaxGatherers": 2
};

let cmpResourceSupply = ConstructComponent(entity, "ResourceSupply", template);

TS_ASSERT(!cmpResourceSupply.IsInfinite());

TS_ASSERT(!cmpResourceSupply.GetKillBeforeGather());

TS_ASSERT_EQUALS(cmpResourceSupply.GetMaxAmount(), 1000);

TS_ASSERT_EQUALS(cmpResourceSupply.GetMaxGatherers(), 2);

TS_ASSERT_EQUALS(cmpResourceSupply.GetDiminishingReturns(), null);

TS_ASSERT_EQUALS(cmpResourceSupply.GetNumGatherers(), 0);

TS_ASSERT(cmpResourceSupply.IsAvailable(1, 70));
TS_ASSERT(cmpResourceSupply.AddGatherer(1, 70));
TS_ASSERT_EQUALS(cmpResourceSupply.GetNumGatherers(), 1);

TS_ASSERT(cmpResourceSupply.AddGatherer(1, 71));
TS_ASSERT_EQUALS(cmpResourceSupply.GetNumGatherers(), 2);

TS_ASSERT(!cmpResourceSupply.AddGatherer(2, 72));
TS_ASSERT_EQUALS(cmpResourceSupply.GetNumGatherers(), 2);

TS_ASSERT(cmpResourceSupply.IsAvailable(1, 70));
TS_ASSERT(!cmpResourceSupply.IsAvailable(1, 73));
TS_ASSERT(!cmpResourceSupply.AddGatherer(1, 73));
TS_ASSERT_EQUALS(cmpResourceSupply.GetNumGatherers(), 2);

cmpResourceSupply.RemoveGatherer(70, 1);
TS_ASSERT_EQUALS(cmpResourceSupply.GetNumGatherers(), 1);

TS_ASSERT_UNEVAL_EQUALS(cmpResourceSupply.GetCurrentAmount(), 1000);
TS_ASSERT_UNEVAL_EQUALS(cmpResourceSupply.TakeResources(300), { "amount": 300, "exhausted": false });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 700);
TS_ASSERT(cmpResourceSupply.IsAvailable(1, 70));

TS_ASSERT_UNEVAL_EQUALS(cmpResourceSupply.TakeResources(800), { "amount": 700, "exhausted": true });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 0);
// The resource is not available when exhausted
TS_ASSERT(!cmpResourceSupply.IsAvailable(1, 70));
