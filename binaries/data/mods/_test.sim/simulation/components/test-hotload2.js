function HotloadA() {}

HotloadA.prototype.Init = function() {
	this.x = +this.template.x;
};

HotloadA.prototype.GetX = function() {
	return this.x*10;
};

Engine.RegisterComponentType(IID_Test1, "HotloadA", HotloadA);
