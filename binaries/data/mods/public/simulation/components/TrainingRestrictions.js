function TrainingRestrictions() {}

TrainingRestrictions.prototype.Schema =
	"<a:help>Specifies unit training restrictions, currently only unit category.</a:help>" +
	"<a:example>" +
		"<TrainingRestrictions>" +
			"<Category>Hero</Category>" +
		"</TrainingRestrictions>" +
	"</a:example>" +
	"<element name='Category' a:help='Specifies the category of this unit, for satisfying special constraints.'>" +
		"<choice>" +
			"<value>Hero</value>" +
			"<value>FemaleCitizen</value>" +
			"<value>WarDog</value>" +
		"</choice>" +
	"</element>";

TrainingRestrictions.prototype.GetCategory = function()
{
	return this.template.Category;
};

Engine.RegisterComponentType(IID_TrainingRestrictions, "TrainingRestrictions", TrainingRestrictions);
