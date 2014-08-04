const VIS_HIDDEN = 0;
const VIS_FOGGED = 1;
const VIS_VISIBLE = 2;

function Fogging() {}

Fogging.prototype.Schema =
	"<a:help>Allows this entity to be replaced by mirages entities in the fog-of-war.</a:help>" +
	"<empty/>";

Fogging.prototype.Init = function()
{
	this.mirages = [];
	this.seen = [];

	var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	for (var player = 0; player < cmpPlayerManager.GetNumPlayers(); ++player)
	{
		this.mirages.push(INVALID_ENTITY);
		this.seen.push(false);
	}
};

Fogging.prototype.LoadMirage = function(player)
{
	var cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	var templateName = "mirage|" + cmpTemplateManager.GetCurrentTemplateName(this.entity);
	
	// If this is an entity without visibility (e.g. a foundation), it should be
	// marked as seen for its owner	
	var cmpParentOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (cmpParentOwnership && cmpParentOwnership.GetOwner() == player)
		this.seen[player] = true;

	if (!this.seen[player] || this.mirages[player] != INVALID_ENTITY)
		return;

	this.mirages[player] = Engine.AddEntity(templateName);
	var cmpMirage = Engine.QueryInterface(this.mirages[player], IID_Mirage);
	if (!cmpMirage)
	{
		error("Failed to load mirage entity for template " + templateName);
		this.mirages[player] = INVALID_ENTITY;
		return;
	}

	// Setup basic mirage properties
	cmpMirage.SetPlayer(player);
	cmpMirage.SetParent(this.entity);

	// Copy cmpOwnership data
	var cmpMirageOwnership = Engine.QueryInterface(this.mirages[player], IID_Ownership);
	if (!cmpParentOwnership || !cmpMirageOwnership)
	{
		error("Failed to setup the ownership data of the fogged entity " + templateName);
		return;
	}
	cmpMirageOwnership.SetOwner(cmpParentOwnership.GetOwner());

	// Copy cmpPosition data
	var cmpParentPosition = Engine.QueryInterface(this.entity, IID_Position);
	var cmpMiragePosition = Engine.QueryInterface(this.mirages[player], IID_Position);
	if (!cmpParentPosition || !cmpMiragePosition)
	{
		error("Failed to setup the position data of the fogged entity " + templateName);
		return;
	}
	if (!cmpParentPosition.IsInWorld())
		return;
	var pos = cmpParentPosition.GetPosition();
	cmpMiragePosition.JumpTo(pos.x, pos.z);
	var rot = cmpParentPosition.GetRotation();
	cmpMiragePosition.SetYRotation(rot.y);
	cmpMiragePosition.SetXZRotation(rot.x, rot.z);

	// Copy cmpVisualActor data
	var cmpParentVisualActor = Engine.QueryInterface(this.entity, IID_Visual);
	var cmpMirageVisualActor = Engine.QueryInterface(this.mirages[player], IID_Visual);
	if (!cmpParentVisualActor || !cmpMirageVisualActor)
	{
		error("Failed to setup the visual data of the fogged entity " + templateName);
		return;
	}
	cmpMirageVisualActor.SetActorSeed(cmpParentVisualActor.GetActorSeed());

	// Store valuable information into the mirage component (especially for the GUI)
	var cmpFoundation = Engine.QueryInterface(this.entity, IID_Foundation);
	if (cmpFoundation)
		cmpMirage.AddFoundation(cmpFoundation.GetBuildPercentage());

	var cmpHealth = Engine.QueryInterface(this.entity, IID_Health);
	if (cmpHealth)
		cmpMirage.AddHealth(
			cmpHealth.GetMaxHitpoints(), 
			cmpHealth.GetHitpoints(), 
			cmpHealth.IsRepairable() && (cmpHealth.GetHitpoints() < cmpHealth.GetMaxHitpoints())
		);

	var cmpResourceSupply = Engine.QueryInterface(this.entity, IID_ResourceSupply);
	if (cmpResourceSupply)
		cmpMirage.AddResourceSupply(
			cmpResourceSupply.GetMaxAmount(), 
			cmpResourceSupply.GetCurrentAmount(), 
			cmpResourceSupply.GetType(), 
			cmpResourceSupply.IsInfinite()
		);
};

Fogging.prototype.IsMiraged = function(player)
{
	if (player >= this.mirages.length)
		return false;

	return this.mirages[player] != INVALID_ENTITY;
};

Fogging.prototype.GetMirage = function(player)
{
	if (player >= this.mirages.length)
		return INVALID_ENTITY;

	return this.mirages[player];
};

Fogging.prototype.WasSeen = function(player)
{
	if (player >= this.seen.length)
		return false;

	return this.seen[player];
};

Fogging.prototype.OnVisibilityChanged = function(msg)
{
	if (msg.player >= this.mirages.length)
		return;

	if (msg.newVisibility == VIS_VISIBLE)
	{
		this.seen[msg.player] = true;

		// Destroy mirages when we get back into LoS
		if (this.mirages[msg.player] != INVALID_ENTITY)
		{
			Engine.DestroyEntity(this.mirages[msg.player]);
			this.mirages[msg.player] = INVALID_ENTITY;
		}
	}

	// Intermediate LoS state, meaning we must create a mirage
	if (msg.newVisibility == VIS_FOGGED)
		this.LoadMirage(msg.player);
};

Fogging.prototype.OnDestroy = function(msg)
{
	for (var player = 0; player < this.mirages.length; ++player)
	{
		var cmpMirage = Engine.QueryInterface(this.mirages[player], IID_Mirage);
		if (cmpMirage)
			cmpMirage.SetParent(INVALID_ENTITY);
	}
};

Engine.RegisterComponentType(IID_Fogging, "Fogging", Fogging);
