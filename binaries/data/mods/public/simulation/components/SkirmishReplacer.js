function SkirmishReplacer() {}

SkirmishReplacer.prototype.Schema = 
	"<optional>" +
		"<oneOrMore>" +
			"<element a:help='Replacement template for the civ which this element is named after or general. If no element is defined for a civ the general element is used instead. If this element is empty the entity is just deleted. The general element gets used if no civ specific element is present and replaces {civ} with the civ code.'>" +
				"<anyName/>" +
				"<interleave>" +
					"<text/>" +
				"</interleave>" +
			"</element>" +
		"</oneOrMore>" +
	"</optional>";

SkirmishReplacer.prototype.Init = function()
{
};

SkirmishReplacer.prototype.OnOwnershipChanged = function(msg)
{
	if (msg.to == 0)
		warn("Skirmish map elements can only be owned by regular players. Please delete entity "+this.entity+" or change the ownership to a non-gaia player.");
};

/**
 * Replace this entity with a civ-specific entity on the first turn
 */
SkirmishReplacer.prototype.OnUpdate = function(msg)
{
	var cmpPlayer = QueryOwnerInterface(this.entity, IID_Player);
	var civ = cmpPlayer.GetCiv();

	var templateName = "";
	if (civ in this.template)
		templateName = this.template[civ];
	else if ("general" in this.template)
		templateName = this.template.general;

	if (!templateName || civ == "gaia")
	{
		Engine.DestroyEntity(this.entity);
		return;
	}
	
	templateName = templateName.replace(/\{civ\}/g, civ);

	var cmpCurPosition = Engine.QueryInterface(this.entity, IID_Position);
	var replacement = Engine.AddEntity(templateName);
	var cmpReplacementPosition = Engine.QueryInterface(replacement, IID_Position)
	var pos = cmpCurPosition.GetPosition2D();
	cmpReplacementPosition.JumpTo(pos.x, pos.y);
	var rot = cmpCurPosition.GetRotation();
	cmpReplacementPosition.SetYRotation(rot.y);
	var cmpCurOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	var cmpReplacementOwnership = Engine.QueryInterface(replacement, IID_Ownership);
	cmpReplacementOwnership.SetOwner(cmpCurOwnership.GetOwner());

	Engine.DestroyEntity(this.entity);
};

Engine.RegisterComponentType(IID_SkirmishReplacer, "SkirmishReplacer", SkirmishReplacer);
