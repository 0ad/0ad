function LoadMapSettings(settings)
{
	// Default settings for old maps
	if (!settings)
		settings = {};

	if (settings.DefaultStance)
	{
		for each (var ent in Engine.GetEntitiesWithInterface(IID_UnitAI))
		{
			var cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
			cmpUnitAI.SetStance(settings.DefaultStance);
		}
	}
}

Engine.RegisterGlobal("LoadMapSettings", LoadMapSettings);
