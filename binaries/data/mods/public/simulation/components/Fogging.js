const VIS_HIDDEN = 0;
const VIS_FOGGED = 1;
const VIS_VISIBLE = 2;

function Fogging() {}

Fogging.prototype.Schema =
	"<a:help>Allows this entity to be replaced by mirage entities in the fog-of-war.</a:help>" +
	"<empty/>";

/**
 * The components that we want to mirage when present.
 * Assumes that a function "Mirage()" is present.
 */
Fogging.prototype.componentsToMirage = [
	IID_Capturable,
	IID_Foundation,
	IID_Health,
	IID_Identity,
	IID_Market,
	IID_Repairable,
	IID_Resistance,
	IID_ResourceSupply
];

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
		// Load a mirage for each player who has already seen the entity.
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
		this.mirages[player] = Engine.AddEntity("mirage|" + Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager).GetCurrentTemplateName(this.entity));

	let cmpMirage = Engine.QueryInterface(this.mirages[player], IID_Mirage);
	if (!cmpMirage)
	{
		error("Failed to load a mirage for entity " + this.entity);
		this.mirages[player] = INVALID_ENTITY;
		return;
	}

	// Copy basic mirage properties.
	cmpMirage.SetPlayer(player);
	cmpMirage.SetParent(this.entity);

	let cmpParentOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	let cmpMirageOwnership = Engine.QueryInterface(this.mirages[player], IID_Ownership);
	if (!cmpParentOwnership || !cmpMirageOwnership)
	{
		error("Failed to copy the ownership data of the fogged entity " + this.entity);
		return;
	}
	cmpMirageOwnership.SetOwner(cmpParentOwnership.GetOwner());

	let cmpParentPosition = Engine.QueryInterface(this.entity, IID_Position);
	let cmpMiragePosition = Engine.QueryInterface(this.mirages[player], IID_Position);
	if (!cmpParentPosition || !cmpMiragePosition)
	{
		error("Failed to copy the position data of the fogged entity " + this.entity);
		return;
	}
	if (!cmpParentPosition.IsInWorld())
		return;
	let pos = cmpParentPosition.GetPosition();
	cmpMiragePosition.JumpTo(pos.x, pos.z);
	let rot = cmpParentPosition.GetRotation();
	cmpMiragePosition.SetYRotation(rot.y);
	cmpMiragePosition.SetXZRotation(rot.x, rot.z);

	let cmpParentVisualActor = Engine.QueryInterface(this.entity, IID_Visual);
	let cmpMirageVisualActor = Engine.QueryInterface(this.mirages[player], IID_Visual);
	if (!cmpParentVisualActor || !cmpMirageVisualActor)
	{
		error("Failed to copy the visual data of the fogged entity " + this.entity);
		return;
	}

	cmpMirageVisualActor.RecomputeActorName();
	cmpMirageVisualActor.SetActorSeed(cmpParentVisualActor.GetActorSeed());

	// Store valuable information into the mirage component (especially for the GUI).
	for (let component of this.componentsToMirage)
		cmpMirage.CopyComponent(component);

	// Notify the GUI the entity has been replaced by a mirage, in case it is selected at this moment.
	Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface).AddMiragedEntity(player, this.entity, this.mirages[player]);

	// Notify the range manager the visibility of this entity must be updated.
	Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager).RequestVisibilityUpdate(this.entity);
};

/**
 * Should only be called for entities that are currently explored, but not visible or
 * things will 'bug out' (double entities and such).
 */
Fogging.prototype.ForceMiraging = function(player)
{
	this.seen[player] = true;

	if (!this.activated)
		return;

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

Fogging.prototype.OnOwnershipChanged = function(msg)
{
	// Always activate fogging for non-Gaia entities.
	if (msg.to > 0)
		this.Activate();

	if (msg.to != -1)
		return;

	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	for (let player = 0; player < this.mirages.length; ++player)
	{
		if (this.mirages[player] == INVALID_ENTITY)
			continue;

		// When this.entity is in the line of sight of the player, its mirage is hidden, rather than destroyed, to save on performance.
		// All hidden mirages can be destroyed now (they won't be needed again), and other mirages will destroy themselves when they get out of the fog.
		if (cmpRangeManager.GetLosVisibility(this.mirages[player], player) == "hidden")
		{
			Engine.DestroyEntity(this.mirages[player]);
			continue;
		}

		let cmpMirage = Engine.QueryInterface(this.mirages[player], IID_Mirage);
		if (cmpMirage)
			cmpMirage.SetParent(INVALID_ENTITY);
	}
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
