function AIProxy() {}

AIProxy.prototype.Schema =
	"<empty/>";

AIProxy.prototype.Init = function()
{
};

AIProxy.prototype.GetRepresentation = function()
{
	// Currently we'll just return the same data that the GUI uses.
	// Maybe we should add/remove things (or make it more efficient)
	// later.

	var cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
	return cmpGuiInterface.GetEntityState(-1, this.entity);
};

Engine.RegisterComponentType(IID_AIProxy, "AIProxy", AIProxy);
