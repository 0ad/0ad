function Wonder() {}

Wonder.prototype.Schema = 
	"<element name='TimeTillVictory'>" +
		"<data type='nonNegativeInteger'/>" +
	"</element>";

Wonder.prototype.Init = function()
{
};

Wonder.prototype.Serialize = null;

Wonder.prototype.GetTimeTillVictory = function()
{
	return +this.template.TimeTillVictory;
};

Engine.RegisterComponentType(IID_Wonder, "Wonder", Wonder);
