function Sound() {}

Sound.prototype.Schema =
	"<element name='SoundGroups'>" +
		"<zeroOrMore>" +
			"<element>" +
				"<anyName/>" +
				"<text/>" +
			"</element>" +
		"</zeroOrMore>" +
	"</element>";

Sound.prototype.Init = function()
{
};

Sound.prototype.GetSoundGroup = function(name)
{
	return this.template.SoundGroups[name] || "";
};

Sound.prototype.PlaySoundGroup = function(name)
{
	if (name in this.template.SoundGroups)
	{
		var cmpSoundManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_SoundManager);
		if (cmpSoundManager)
			cmpSoundManager.PlaySoundGroup(this.template.SoundGroups[name], this.entity);
	}
};

Engine.RegisterComponentType(IID_Sound, "Sound", Sound);
