function RallyPoint() {}

RallyPoint.prototype.Schema =
	"<a:component/><empty/>";

RallyPoint.prototype.Init = function()
{
	this.pos = [];
	this.data = [];
};

RallyPoint.prototype.AddPosition = function(x, z)
{
	this.pos.push({
		"x": x,
		"z": z
	});
};

RallyPoint.prototype.GetPositions = function()
{
	return this.pos;
};

// Extra data for the rally point, should have a command property and then helpful data for that command
// See getActionInfo in gui/input.js
RallyPoint.prototype.AddData = function(data)
{
	this.data.push(data);
};

// Returns an array with the data associated with this rally point.  Each element has the structure:
// {"type": "walk/gather/garrison/...", "target": targetEntityId, "resourceType": "tree/fruit/ore/..."} where target 
// and resourceType (specific resource type) are optional, also target may be an invalid entity, check for existence.
RallyPoint.prototype.GetData = function()
{
	return this.data;
};

RallyPoint.prototype.Unset = function()
{
	this.pos = [];
	this.data = [];
};

RallyPoint.prototype.Reset = function()
{
	this.Unset();
	var cmpRallyPointRenderer = Engine.QueryInterface(this.entity, IID_RallyPointRenderer);
	if (cmpRallyPointRenderer)
		cmpRallyPointRenderer.Reset();
};

Engine.RegisterComponentType(IID_RallyPoint, "RallyPoint", RallyPoint);
