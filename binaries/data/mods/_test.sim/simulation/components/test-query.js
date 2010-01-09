function TestScript1A() {}

TestScript1A.prototype.Init = function() {
	this.x = 100;
};

TestScript1A.prototype.GetX = function() {
	var test2 = Engine.QueryInterface(this.entity, IID_Test2);
	return test2.GetX() + (test2.x || 0);
};

Engine.RegisterComponentType(IID_Test1, "TestScript1A", TestScript1A);

// -------- //

function TestScript2A() {}

TestScript2A.prototype.Init = function() {
	this.x = 200;
};

TestScript2A.prototype.GetX = function() {
	return this.x;
};

Engine.RegisterComponentType(IID_Test2, "TestScript2A", TestScript2A);
