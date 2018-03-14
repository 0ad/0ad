function SkirmishReplacer() {}

SkirmishReplacer.prototype.Schema =
		"<optional>" +
			"<element name='general' a:help='The general element replaces {civ} with the civ code.'>" +
				"<interleave>" +
					"<text/>" +
				"</interleave>" +
			"</element>" +
		"</optional>";

SkirmishReplacer.prototype.Init = function()
{
};

SkirmishReplacer.prototype.Serialize = null; // We have no dynamic state to save

function getReplacementEntities(civ)
{
	return Engine.ReadJSONFile("simulation/data/civs/" + civ + ".json").SkirmishReplacements;
}

SkirmishReplacer.prototype.OnOwnershipChanged = function(msg)
{
	if (msg.to == 0)
		warn("Skirmish map elements can only be owned by regular players. Please delete entity "+this.entity+" or change the ownership to a non-gaia player.");
};

SkirmishReplacer.prototype.ReplaceEntities = function()
{
	var cmpPlayer = QueryOwnerInterface(this.entity);
	var civ = cmpPlayer.GetCiv();
	var replacementEntities = getReplacementEntities(civ);

	var cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	var templateName = cmpTemplateManager.GetCurrentTemplateName(this.entity);

	let specialFilters = templateName.substr(0, templateName.lastIndexOf("|") + 1)
	templateName = removeFiltersFromTemplateName(templateName);

	if (templateName in replacementEntities)
		templateName = replacementEntities[templateName];
	else if (this.template && "general" in this.template)
		templateName = this.template.general;
	else
		templateName = "";

	if (!templateName || civ == "gaia")
	{
		Engine.DestroyEntity(this.entity);
		return;
	}

	templateName = specialFilters + templateName.replace(/\{civ\}/g, civ);

	var cmpCurPosition = Engine.QueryInterface(this.entity, IID_Position);
	var replacement = Engine.AddEntity(templateName);
	if (!replacement)
	{
		Engine.DestroyEntity(this.entity);
		return;
	}
	var cmpReplacementPosition = Engine.QueryInterface(replacement, IID_Position);
	var pos = cmpCurPosition.GetPosition2D();
	cmpReplacementPosition.JumpTo(pos.x, pos.y);
	var rot = cmpCurPosition.GetRotation();
	cmpReplacementPosition.SetYRotation(rot.y);
	var cmpCurOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	var cmpReplacementOwnership = Engine.QueryInterface(replacement, IID_Ownership);
	cmpReplacementOwnership.SetOwner(cmpCurOwnership.GetOwner());

	Engine.PostMessage(this.entity, MT_EntityRenamed, { "entity": this.entity, "newentity": replacement });
	Engine.DestroyEntity(this.entity);
};

/**
 * Replace this entity with a civ-specific entity
 * Message is sent right before InitGame() is called, in InitGame.js
 * Replacement needs to happen early on real games to not confuse the AI
 */
SkirmishReplacer.prototype.OnSkirmishReplace = function(msg)
{
	this.ReplaceEntities();
};

/**
 * Replace this entity with a civ-specific entity
 * This is needed for Atlas, when the entity isn't replaced before the game starts,
 * so it needs to be replaced on the first turn.
 */
SkirmishReplacer.prototype.OnUpdate = function(msg)
{
	this.ReplaceEntities();
};

Engine.RegisterComponentType(IID_SkirmishReplacer, "SkirmishReplacer", SkirmishReplacer);
