function TerritoryDecayManager() {}

TerritoryDecayManager.prototype.Schema =
        "<a:component type='system'/><empty/>";

TerritoryDecayManager.prototype.SetBlinkingEntities = function()
{
	for (let ent of Engine.GetEntitiesWithInterface(IID_TerritoryDecay))
		Engine.QueryInterface(ent, IID_TerritoryDecay).IsConnected();
};

Engine.RegisterSystemComponentType(IID_TerritoryDecayManager, "TerritoryDecayManager", TerritoryDecayManager);
