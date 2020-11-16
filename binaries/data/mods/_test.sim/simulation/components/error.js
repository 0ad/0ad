function TestScript1A() {}

TS_ASSERT_EXCEPTION(() => { Engine.RegisterComponentType(12345, "TestScript1A", TestScript1A); });

var n = Engine.QueryInterface(12345, IID_Test1);
if (n !== null)
	Engine.TS_FAIL("QueryInterface return "+n+", not null");
