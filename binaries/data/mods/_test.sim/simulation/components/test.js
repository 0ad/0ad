function TestScript1A() {}

TestScript1A.prototype.Init = function() {
	this.x = 101000;
};

TestScript1A.prototype.GetX = function() {
	return this.x;
};

TestScript1A.prototype.OnTurnStart = function(msg) {
	this.x += 1;
};

Engine.RegisterComponentType(IID_Test1, "TestScript1A", TestScript1A);

// -------- //

function TestScript1B() {}

TestScript1B.prototype = Object.create(TestScript1A.prototype);

TestScript1B.prototype.Init = function() {
	this.x = 102000;
};

Engine.RegisterComponentType(IID_Test1, "TestScript1B", TestScript1B);

// -------- //

function TestScript2A() {}

TestScript2A.prototype.Init = function() {
	this.x = 201000;
};

TestScript2A.prototype.GetX = function() {
	return this.x;
};

TestScript2A.prototype.OnUpdate = function(msg) {
	this.x += msg.turnLength;
};

Engine.RegisterComponentType(IID_Test2, "TestScript2A", TestScript2A);
