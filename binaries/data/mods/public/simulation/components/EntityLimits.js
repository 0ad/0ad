function EntityLimits() {}

EntityLimits.prototype.Schema =
	"<a:help>Specifies per category limits on number of entities (buildings or units) that can be created for each player</a:help>" +
	"<a:example>" +
		"<Limits>" +
			"<DefenseTower>25</DefenseTower>" +
			"<Fortress>10</Fortress>" +
			"<Wonder>1</Wonder>" +
			"<Hero>1</Hero>" +
			"<Apadana>1</Apadana>" +
			"<Monument>5</Monument>" +
		"</Limits>" +
		"<LimitChangers>" +
			"<Monument>" +
				"<CivCentre>2</CivCentre>" +
			"</Monument>" +
		"</LimitChangers>" +
	"</a:example>" +
	"<element name='Limits'>" +
		"<zeroOrMore>" +
			"<element a:help='Specifies a category of building/unit on which to apply this limit. See BuildRestrictions/TrainingRestrictions for possible categories'>" +
				"<anyName />" +
				"<data type='integer'/>" +
			"</element>" +
		"</zeroOrMore>" +
	"</element>" +
	"<element name='LimitChangers'>" +
		"<zeroOrMore>" +
			"<element a:help='Specifies a category of building/unit on which to apply this limit. See BuildRestrictions/TrainingRestrictions for possible categories'>" +
				"<anyName />" +
				"<zeroOrMore>" +
					"<element a:help='Specifies the class that changes the entity limit'>" +
						"<anyName />" +
						"<data type='integer'/>" +
					"</element>" +
				"</zeroOrMore>" +
			"</element>" +
		"</zeroOrMore>" +
	"</element>";


/*
 *	TODO: Use an inheriting player_{civ}.xml template for civ-specific limits
 */

const TRAINING = "training";
const BUILD = "build";

EntityLimits.prototype.Init = function()
{
	this.limit = {};
	this.count = {};
	this.changers = {};
	for (var category in this.template.Limits)
	{
		this.limit[category] = +this.template.Limits[category];
		this.count[category] = 0;
		if (!(category in this.template.LimitChangers))
			continue;
		this.changers[category] = {};
		for (var c in this.template.LimitChangers[category])
			this.changers[category][c] = +this.template.LimitChangers[category][c];
	}
};

EntityLimits.prototype.ChangeLimit = function(category, value)
{
	this.limit[category] += value;
};

EntityLimits.prototype.ChangeCount = function(category, value)
{
	if (this.count[category] !== undefined)
		this.count[category] += value;
};

EntityLimits.prototype.GetLimits = function()
{
	return this.limit;
};

EntityLimits.prototype.GetCounts = function()
{
	return this.count;
};

EntityLimits.prototype.GetLimitChangers = function()
{
	return this.changers;
};

EntityLimits.prototype.AllowedToCreate = function(limitType, category, count)
{
	// Allow unspecified categories and those with no limit
	if (this.count[category] === undefined || this.limit[category] === undefined)
		return true;
	
	if (this.count[category] + count > this.limit[category])
	{
		var cmpPlayer = Engine.QueryInterface(this.entity, IID_Player);
		var notification = {
			"player": cmpPlayer.GetPlayerID(),
			"translateMessage": true,
			"translateParameters": ["category"],
			"parameters": {"category": category, "limit": this.limit[category]},
		};

		if (limitType == BUILD)
			notification.message = markForTranslation("%(category)s build limit of %(limit)s reached");
		else if (limitType == TRAINING)
			notification.message = markForTranslation("%(category)s training limit of %(limit)s reached");
		else
		{
			warn("EntityLimits.js: Unknown LimitType " + limitType)
			notification.message = markForTranslation("%(category)s limit of %(limit)s reached");
		}
		var cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
		cmpGUIInterface.PushNotification(notification);
		
		return false;
	}
	
	return true;
}

EntityLimits.prototype.AllowedToBuild = function(category)
{
	// We pass count 0 as the creation of the building has already taken place and
	// the ownership has been set (triggering OnGlobalOwnershipChanged) 
	return this.AllowedToCreate(BUILD, category, 0);
};

EntityLimits.prototype.AllowedToTrain = function(category, count)
{
	return this.AllowedToCreate(TRAINING, category, count);
};

EntityLimits.prototype.OnGlobalOwnershipChanged = function(msg)
{	
	// check if we are adding or removing an entity from this player
	var cmpPlayer = Engine.QueryInterface(this.entity, IID_Player);
	if (!cmpPlayer)
	{
		error("EntityLimits component is defined on a non-player entity");
		return;
	}
	if (msg.from == cmpPlayer.GetPlayerID())
		var modifier = -1;
	else if (msg.to == cmpPlayer.GetPlayerID())
		var modifier = 1;
	else 
		return;

	// Update entity counts
	var category = null;
	var cmpBuildRestrictions = Engine.QueryInterface(msg.entity, IID_BuildRestrictions);
	if (cmpBuildRestrictions)
		category = cmpBuildRestrictions.GetCategory();
	var cmpTrainingRestrictions = Engine.QueryInterface(msg.entity, IID_TrainingRestrictions);
	if (cmpTrainingRestrictions)
		category = cmpTrainingRestrictions.GetCategory();
	if (category)
		this.ChangeCount(category,modifier);

	// Update entity limits
	var cmpIdentity = Engine.QueryInterface(msg.entity, IID_Identity);
	if (!cmpIdentity)
		return;
	// foundations shouldn't change the entity limits until they're completed
	var cmpFoundation = Engine.QueryInterface(msg.entity, IID_Foundation);
	if (cmpFoundation)
		return;
	var classes = cmpIdentity.GetClassesList();
	for (var category in this.changers)
		for (var c in this.changers[category])
			if (classes.indexOf(c) >= 0)
				this.ChangeLimit(category, modifier * this.changers[category][c]);
};

Engine.RegisterComponentType(IID_EntityLimits, "EntityLimits", EntityLimits);
