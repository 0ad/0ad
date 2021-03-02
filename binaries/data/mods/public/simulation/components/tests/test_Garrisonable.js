Engine.LoadComponentScript("interfaces/Auras.js");
Engine.LoadComponentScript("interfaces/Garrisonable.js");
Engine.LoadComponentScript("interfaces/GarrisonHolder.js");
Engine.LoadComponentScript("interfaces/ProductionQueue.js");
Engine.LoadComponentScript("interfaces/UnitAI.js");
Engine.LoadComponentScript("Garrisonable.js");

Engine.RegisterGlobal("ApplyValueModificationsToEntity", (prop, oVal, ent) => oVal);

const garrisonHolderID = 1;
const garrisonableID = 2;
AddMock(garrisonHolderID, IID_GarrisonHolder, {
	"Garrison": () => true
});

let size = 1;
let cmpGarrisonable = ConstructComponent(garrisonableID, "Garrisonable", {
	"Size": size
});

TS_ASSERT_EQUALS(cmpGarrisonable.UnitSize(garrisonHolderID), size);
TS_ASSERT_EQUALS(cmpGarrisonable.TotalSize(garrisonHolderID), size);

let extraSize = 2;
AddMock(garrisonableID, IID_GarrisonHolder, {
	"OccupiedSlots": () => extraSize
});

TS_ASSERT_EQUALS(cmpGarrisonable.UnitSize(garrisonHolderID), size);
TS_ASSERT_EQUALS(cmpGarrisonable.TotalSize(garrisonHolderID), size + extraSize);

TS_ASSERT(cmpGarrisonable.Garrison(garrisonHolderID));
TS_ASSERT_UNEVAL_EQUALS(cmpGarrisonable.HolderID(), garrisonHolderID);

TS_ASSERT(!cmpGarrisonable.Garrison(garrisonHolderID));
TS_ASSERT_UNEVAL_EQUALS(cmpGarrisonable.HolderID(), garrisonHolderID);

cmpGarrisonable.UnGarrison();
TS_ASSERT_UNEVAL_EQUALS(cmpGarrisonable.HolderID(), INVALID_ENTITY);
