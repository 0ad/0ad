function VisionSharing() {}

VisionSharing.prototype.Schema =
	"<empty/>";

VisionSharing.prototype.Init = function()
{
	this.activated = false;
	this.shared = new Set();
};

/**
 * As entities have not necessarily the VisionSharing component, it has to be activated
 * before use so that the rangeManager can register it
 */
VisionSharing.prototype.Activate = function()
{
	if (this.activated)
		return;
	let cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (!cmpOwnership || cmpOwnership.GetOwner() <= 0)
		return;
	this.shared.add(cmpOwnership.GetOwner());
	Engine.PostMessage(this.entity, MT_VisionSharingChanged,
		{ "entity": this.entity, "player": cmpOwnership.GetOwner(), "add": true });
	this.activated = true;
};

VisionSharing.prototype.CheckVisionSharings = function()
{
	let shared = new Set();

	let cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	let owner = cmpOwnership ? cmpOwnership.GetOwner() : -1;
	if (owner >= 0)
	{
		// The owner has vision
		if (owner > 0)		
			shared.add(owner);

		// Vision sharing due to garrisoned units
		let cmpGarrisonHolder = Engine.QueryInterface(this.entity, IID_GarrisonHolder);
		if (cmpGarrisonHolder)
		{
			for (let ent of cmpGarrisonHolder.GetEntities())
			{
				let cmpEntOwnership = Engine.QueryInterface(ent, IID_Ownership); 
				if (!cmpEntOwnership)
					continue;
				let entOwner = cmpEntOwnership.GetOwner();
				if (entOwner > 0 && entOwner != owner)
				{
					shared.add(entOwner);
					// if shared by another player than the owner and not yet activated, do it
					this.Activate();
				}
			}
		}
	}

	if (!this.activated)
		return;

	// compare with previous vision sharing, and update if needed
	for (let player of shared)
		if (!this.shared.has(player))
			Engine.PostMessage(this.entity, MT_VisionSharingChanged,
				{ "entity": this.entity, "player": player, "add": true });
	for (let player of this.shared)
		if (!shared.has(player))
			Engine.PostMessage(this.entity, MT_VisionSharingChanged,
				{ "entity": this.entity, "player": player, "add": false });
	this.shared = shared;
};

VisionSharing.prototype.OnDiplomacyChanged = function(msg)
{
	this.CheckVisionSharings();
};

VisionSharing.prototype.OnGarrisonedUnitsChanged = function(msg)
{
	this.CheckVisionSharings();
};

VisionSharing.prototype.OnOwnershipChanged = function(msg)
{
	if (this.activated)
		this.CheckVisionSharings();
};

Engine.RegisterComponentType(IID_VisionSharing, "VisionSharing", VisionSharing);
