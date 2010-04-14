function HotloadA() {}

HotloadA.prototype.Schema = "<ref name='anything'/>";

HotloadA.prototype.Init = function() {
	this.x = +this.template.x;
};

HotloadA.prototype.GetX = function() {
	return this.x*10;
};

Engine.RegisterComponentType(IID_Test1, "HotloadA", HotloadA);


function HotloadC() {}

Engine.RegisterInterface("HotloadInterface");
Engine.RegisterComponentType(IID_HotloadInterface, "HotloadC", HotloadC);


Engine.RegisterGlobal("HotloadGlobal", 2);
