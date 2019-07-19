function Modding() {}

Modding.prototype.Schema = "<ref name='anything'/>";

Modding.prototype.Init = function() {
	this.x = +this.template.x;
};

Modding.prototype.GetX = function() {
	return this.x;
};

Engine.RegisterComponentType(IID_Test1, "Modding", Modding);
