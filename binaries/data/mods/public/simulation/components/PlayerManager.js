function PlayerManager() {}

PlayerManager.prototype.Schema =
	"<a:component type='system'/><empty/>";

// Diplomatic stance constants
PlayerManager.prototype.Diplomacy = {
	"ENEMY" : -1,
	"NEUTRAL" : 0,
	"ALLY" : 1
};
	
PlayerManager.prototype.Init = function()
{
	this.playerEntities = []; // list of player entity IDs
};

PlayerManager.prototype.AddPlayer = function(ent)
{
	var id = this.playerEntities.length;
	Engine.QueryInterface(ent, IID_Player).SetPlayerID(id);
	this.playerEntities.push(ent);
	return id;
};

PlayerManager.prototype.GetPlayerByID = function(id)
{
	return this.playerEntities[id];
	// TODO: report error message if invalid id
};

PlayerManager.prototype.GetNumPlayers = function()
{
	return this.playerEntities.length;
};

Engine.RegisterComponentType(IID_PlayerManager, "PlayerManager", PlayerManager);
