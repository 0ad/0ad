Engine.RegisterInterface("TestSetup");

function TestSetup() {};
TestSetup.prototype.Init = function() {};

Engine.RegisterSystemComponentType(IID_TestSetup, "TestSetup", TestSetup);
let cmpTestSetup = ConstructComponent(SYSTEM_ENTITY, "TestSetup", { "property": "value" });

function expectException(func)
{
	try {
		func();
		Engine.TS_FAIL("Missed exception at " + new Error().stack);
	} catch (e) {}
}

expectException(() => { cmpTestSetup.template = "replacement forbidden"; });
expectException(() => { cmpTestSetup.template.property = "modification forbidden"; });
expectException(() => { cmpTestSetup.template.other_property = "insertion forbidden"; });
expectException(() => { delete cmpTestSetup.entity; });
expectException(() => { delete cmpTestSetup.template; });
expectException(() => { delete cmpTestSetup.template.property; });

TS_ASSERT_UNEVAL_EQUALS(cmpTestSetup.template, { "property": "value" });
