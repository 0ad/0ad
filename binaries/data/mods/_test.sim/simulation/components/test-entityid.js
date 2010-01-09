function TestScript1A() {}

TestScript1A.prototype.GetX = function() {
	// Test that .entity is readonly
	delete this.entity;
	this.entity = -1;
	
	// and return the value
	return this.entity;
};

Engine.RegisterComponentType(IID_Test1, "TestScript1A", TestScript1A);
