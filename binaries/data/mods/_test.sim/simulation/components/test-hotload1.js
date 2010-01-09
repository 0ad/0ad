function HotloadA() {}

HotloadA.prototype.Init = function() {
	this.x = +this.template.x;
};

HotloadA.prototype.GetX = function() {
	return this.x;
};

Engine.RegisterComponentType(IID_Test1, "HotloadA", HotloadA);

function HotloadB() {}

HotloadB.prototype.Init = function() {
  this.x = +this.template.x;
};

HotloadB.prototype.GetX = function() {
  return this.x*2;
};

Engine.RegisterComponentType(IID_Test1, "HotloadB", HotloadB);