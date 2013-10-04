const civList = ["general", "athen", "brit", "cart", "celt", "gaul", "hele", "iber", "mace", "maur", "pers", "rome", "spart"];

function SkirmishReplacer() {}


SkirmishReplacer.prototype.Schema = "";
for each (var civ in civList)
	SkirmishReplacer.prototype.Schema += 
		"<optional>" +
			"<element name='"+civ+"' a:help='Replacement template for this civ. If this element is not present, the \"general\" element (with {civ} replaced by the civ code) is taken. If this element is empty, or not defined, and also no general element, this entity is just deleted.'>" +
				"<text/>" +
			"</element>" +
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
