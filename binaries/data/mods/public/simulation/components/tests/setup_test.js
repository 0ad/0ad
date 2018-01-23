Engine.RegisterInterface("TestSetup");

function TestSetup() {};
TestSetup.prototype.Init = function() {};

Engine.RegisterSystemComponentType(IID_TestSetup, "TestSetup", TestSetup);
let cmpTestSetup = ConstructComponent(SYSTEM_ENTITY, "TestSetup", { "property": "value" });

TS_ASSERT_EXCEPTION(() => { cmpTestSetup.template = "replacement forbidden"; });
TS_ASSERT_EXCEPTION(() => { cmpTestSetup.template.property = "modification forbidden"; });
TS_ASSERT_EXCEPTION(() => { cmpTestSetup.template.other_property = "insertion forbidden"; });
TS_ASSERT_EXCEPTION(() => { delete cmpTestSetup.entity; });
TS_ASSERT_EXCEPTION(() => { delete cmpTestSetup.template; });
TS_ASSERT_EXCEPTION(() => { delete cmpTestSetup.template.property; });

TS_ASSERT_UNEVAL_EQUALS(cmpTestSetup.template, { "property": "value" });
