function TestScript1A() {}

try {
	Engine.RegisterComponentType(12345, "TestScript1A", TestScript1A);
	Engine.TS_FAIL("Missed exception");
} catch (e) {
//	print("Caught exception: " + e + "\n");
}

var n = Engine.QueryInterface(12345, IID_Test1);
if (n !== null)
	Engine.TS_FAIL("QueryInterface return "+n+", not null");
