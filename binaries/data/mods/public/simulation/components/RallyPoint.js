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

RallyPoint.prototype.GetPosition = function()
{
	return this.pos;
};

// Extra data for the rally point, should have a command property and then helpful data for that command
// See getActionInfo in gui/input.js
RallyPoint.prototype.SetData = function(data)
{
	this.data = data;
};

// Returns the data associated with this rally point.  Uses the data structure:
// {"type": "walk/gather/garrison/...", "target": targetEntityId, "resourceType": "tree/fruit/ore/..."} where target 
// and resourceType (specific resource type) are optional, also target may be an invalid entity, check for existance.
RallyPoint.prototype.GetData = function()
{
	return this.data;
};

RallyPoint.prototype.Unset = function()
{
	this.pos = undefined;
	this.data = undefined;
};


Engine.RegisterComponentType(IID_RallyPoint, "RallyPoint", RallyPoint);
