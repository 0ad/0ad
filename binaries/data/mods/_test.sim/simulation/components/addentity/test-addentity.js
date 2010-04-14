function TestScript2A() {}

TestScript2A.prototype.Schema = "<ref name='anything'/>";

TestScript2A.prototype.Init = function() {
	this.x = eval(this.template.y);
};

TestScript2A.prototype.GetX = function() {
	return this.x;
};

Engine.RegisterComponentType(IID_Test2, "TestScript2A", TestScript2A);
