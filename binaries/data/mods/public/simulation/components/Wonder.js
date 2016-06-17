function Wonder() {}

Wonder.prototype.Schema = "<a:component type='system'/><empty/>";

Wonder.prototype.Init = function()
{
};

Wonder.prototype.Serialize = null;

Engine.RegisterComponentType(IID_Wonder, "Wonder", Wonder);
