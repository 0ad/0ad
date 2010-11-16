function TestScript1A() {}

TestScript1A.prototype.GetX = function() {
	// Test that .entity is readonly
	try {
		delete this.entity;
		Engine.TS_FAIL("Missed exception");
	} catch (e) { }
	try {
		this.entity = -1;
		Engine.TS_FAIL("Missed exception");
	} catch (e) { }
	
	// and return the value
	return this.entity;
};

Engine.RegisterComponentType(IID_Test1, "TestScript1A", TestScript1A);
