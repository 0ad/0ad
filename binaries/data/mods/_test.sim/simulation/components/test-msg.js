function TestScript1A() {}

TestScript1A.prototype.Init = function() {
	this.x = 100;
};

TestScript1A.prototype.GetX = function() {
	return this.x;
};

TestScript1A.prototype.OnUpdate = function(msg) {
	this.x += msg.turnLength;
};

Engine.RegisterComponentType(IID_Test1, "TestScript1A", TestScript1A);



function TestScript1B() {}

TestScript1B.prototype.Init = function() {
	this.x = 100;
};

TestScript1B.prototype.GetX = function() {
	return this.x;
};

TestScript1B.prototype.OnGlobalUpdate = function(msg) {
	this.x += msg.turnLength;
};

Engine.RegisterComponentType(IID_Test1, "TestScript1B", TestScript1B);



function TestScript2A() {}

TestScript2A.prototype.Init = function() {
	this.x = 200;
};

TestScript2A.prototype.GetX = function() {
	Engine.BroadcastMessage(MT_Update, { turnLength: 50 });
	Engine.PostMessage(1, MT_Update, { turnLength: 500 });
	Engine.PostMessage(2, MT_Update, { turnLength: 5000 });
	return this.x;
};

Engine.RegisterComponentType(IID_Test2, "TestScript2A", TestScript2A);
