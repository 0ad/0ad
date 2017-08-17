const VIS_HIDDEN = 0;
const VIS_FOGGED = 1;
const VIS_VISIBLE = 2;

function Fogging() {}

Fogging.prototype.Schema =
	"<a:help>Allows this entity to be replaced by mirage entities in the fog-of-war.</a:help>" +
	"<empty/>";

Fogging.prototype.Init = function()
{
	this.activated = false;
	this.mirages = [];
	this.miraged = [];
	this.seen = [];

	let numPlayers = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager).GetNumPlayers();
	for (let player = 0; player < numPlayers; ++player)
	{
		this.mirages.push(INVALID_ENTITY);
		this.miraged.push(false);
		this.seen.push(false);
	}
};

Fogging.prototype.Activate = function()
{
	let mustUpdate = !this.activated;
	this.activated = true;

	if (mustUpdate)
	{
		// Load a mirage for each player who has already seen the entity
		let numPlayers = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager).GetNumPlayers();
		let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
		for (let player = 0; player < numPlayers; ++player)
			if (this.seen[player] && cmpRangeManager.GetLosVisibility(this.entity, player) != "visible")
				this.LoadMirage(player);
	}
};

Fogging.prototype.IsActivated = function()
{
	return this.activated;
};

Fogging.prototype.LoadMirage = function(player)
{
	if (!this.activated)
	{
		error("LoadMirage called for an entity with fogging deactivated");
		return;
	}

	this.miraged[player] = true;

	if (this.mirages[player] == INVALID_ENTITY)
	{
		var cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
		var templateName = "mirage|" + cmpTemplateManager.GetCurrentTemplateName(this.entity);

		this.mirages[player] = Engine.AddEntity(templateName);
	}

	var cmpMirage = Engine.QueryInterface(this.mirages[player], IID_Mirage);
	if (!cmpMirage)
	{
		error("Failed to load a mirage for entity " + this.entity);
		this.mirages[player] = INVALID_ENTITY;
		return;
	}

	// Copy basic mirage properties
	cmpMirage.SetPlayer(player);
	cmpMirage.SetParent(this.entity);

	// Copy cmpOwnership data
	var cmpParentOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	var cmpMirageOwnership = Engine.QueryInterface(this.mirages[player], IID_Ownership);
	if (!cmpParentOwnership || !cmpMirageOwnership)
	{
		error("Failed to copy the ownership data of the fogged entity " + this.entity);
		return;
	}
	cmpMirageOwnership.SetOwner(cmpParentOwnership.GetOwner());

	// Copy cmpPosition data
	var cmpParentPosition = Engine.QueryInterface(this.entity, IID_Position);
	var cmpMiragePosition = Engine.QueryInterface(this.mirages[player], IID_Position);
	if (!cmpParentPosition || !cmpMiragePosition)
	{
		error("Failed to copy the position data of the fogged entity " + this.entity);
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
		error("Failed to copy the visual data of the fogged entity " + this.entity);
		return;
	}
	cmpMirageVisualActor.SetActorSeed(cmpParentVisualActor.GetActorSeed());

	// Store valuable information into the mirage component (especially for the GUI)
	var cmpIdentity = Engine.QueryInterface(this.entity, IID_Identity);
	if (cmpIdentity)
		cmpMirage.CopyIdentity(cmpIdentity);

	var cmpFoundation = Engine.QueryInterface(this.entity, IID_Foundation);
	if (cmpFoundation)
		cmpMirage.CopyFoundation(cmpFoundation);

	var cmpHealth = Engine.QueryInterface(this.entity, IID_Health);
	if (cmpHealth)
		cmpMirage.CopyHealth(cmpHealth);

	var cmpCapturable = Engine.QueryInterface(this.entity, IID_Capturable);
	if (cmpCapturable)
		cmpMirage.CopyCapturable(cmpCapturable);

	var cmpResourceSupply = Engine.QueryInterface(this.entity, IID_ResourceSupply);
	if (cmpResourceSupply)
		cmpMirage.CopyResourceSupply(cmpResourceSupply);

	var cmpMarket = Engine.QueryInterface(this.entity, IID_Market);
	if (cmpMarket)
		cmpMirage.CopyMarket(cmpMarket);

	// Notify the GUI the entity has been replaced by a mirage, in case it is selected at this moment
	var cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
	cmpGuiInterface.AddMiragedEntity(player, this.entity, this.mirages[player]);

	// Notify the range manager the visibility of this entity must be updated
	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	cmpRangeManager.RequestVisibilityUpdate(this.entity);
};

Fogging.prototype.ForceMiraging = function(player)
{
	if (!this.activated)
		return;

	this.seen[player] = true;
	this.LoadMirage(player);
};

Fogging.prototype.IsMiraged = function(player)
{
	if (player < 0 || player >= this.mirages.length)
		return false;

	return this.miraged[player];
};

Fogging.prototype.GetMirage = function(player)
{
	if (player < 0 || player >= this.mirages.length)
		return INVALID_ENTITY;

	return this.mirages[player];
};

Fogging.prototype.WasSeen = function(player)
{
	if (player < 0 || player >= this.seen.length)
		return false;

	return this.seen[player];
};

Fogging.prototype.OnDestroy = function(msg)
{
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	for (var player = 0; player < this.mirages.length; ++player)
	{
		if (cmpRangeManager.GetLosVisibility(this.mirages[player], player) == "hidden")
		{
			Engine.DestroyEntity(this.mirages[player]);
			continue;
		}

		var cmpMirage = Engine.QueryInterface(this.mirages[player], IID_Mirage);
		if (cmpMirage)
			cmpMirage.SetParent(INVALID_ENTITY);
	}
};

Fogging.prototype.OnOwnershipChanged = function(msg)
{
	// Always activate fogging for non-Gaia entities
	if (msg.to > 0)
		this.Activate();
};

Fogging.prototype.OnVisibilityChanged = function(msg)
{
	if (msg.player < 0 || msg.player >= this.mirages.length)
		return;

	if (msg.newVisibility == VIS_VISIBLE)
	{
		this.miraged[msg.player] = false;
		this.seen[msg.player] = true;
	}

	if (msg.newVisibility == VIS_FOGGED && this.activated)
		this.LoadMirage(msg.player);
};

Engine.RegisterComponentType(IID_Fogging, "Fogging", Fogging);
