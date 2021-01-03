Engine.LoadComponentScript("interfaces/Garrisonable.js");
Engine.LoadComponentScript("Garrisonable.js");

const garrisonHolderID = 1;
const garrisonableID = 2;

let cmpGarrisonable = ConstructComponent(garrisonableID, "Garrisonable", {
});

TS_ASSERT(cmpGarrisonable.Garrison(garrisonHolderID));
TS_ASSERT_UNEVAL_EQUALS(cmpGarrisonable.HolderID(), garrisonHolderID);

TS_ASSERT(!cmpGarrisonable.Garrison(garrisonHolderID));
TS_ASSERT_UNEVAL_EQUALS(cmpGarrisonable.HolderID(), garrisonHolderID);

cmpGarrisonable.UnGarrison();
TS_ASSERT_UNEVAL_EQUALS(cmpGarrisonable.HolderID(), INVALID_ENTITY);
