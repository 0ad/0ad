function RallyPoint() {}

RallyPoint.prototype.Schema =
	"<a:component/><empty/>";

RallyPoint.prototype.Init = function()
{
	this.pos = undefined;
};

RallyPoint.prototype.SetPosition = function(x, z)
{
	this.pos = {
		"x": x,
		"z": z
	};
};

RallyPoint.prototype.Unset = function()
{
	this.pos = undefined;
};

RallyPoint.prototype.GetPosition = function()
{
	return this.pos;
};

Engine.RegisterComponentType(IID_RallyPoint, "RallyPoint", RallyPoint);
