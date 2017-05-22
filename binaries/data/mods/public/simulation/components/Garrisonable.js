function Garrisonable() {}

Garrisonable.prototype.Schema = "<empty/>";

Garrisonable.prototype.Init = function()
{
};

Garrisonable.prototype.Serialize = null;

Engine.RegisterComponentType(IID_Garrisonable, "Garrisonable", Garrisonable);
