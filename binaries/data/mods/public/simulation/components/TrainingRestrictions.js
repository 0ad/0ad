function TrainingRestrictions() {}

TrainingRestrictions.prototype.Schema =
	"<a:help>Specifies unit training restrictions, currently only unit category</a:help>" +
	"<a:example>" +
		"<TrainingRestrictions>" +
			"<Category>Hero</Category>" +
		"</TrainingRestrictions>" +
	"</a:example>" +
	"<element name='Category' a:help='Specifies the category of this unit, for satisfying special constraints. Choices include: Hero, UniqueBuilding, WarDog'>" +
		"<text/>" +
	"</element>";

TrainingRestrictions.prototype.Serialize = null;

TrainingRestrictions.prototype.GetCategory = function()
{
	return this.template.Category;
};

Engine.RegisterComponentType(IID_TrainingRestrictions, "TrainingRestrictions", TrainingRestrictions);
