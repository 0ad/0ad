function Sound() {}

Sound.prototype.Schema =
	"<a:help>Lists the sound groups associated with this unit.</a:help>" +
	"<a:example>" +
		"<SoundGroups>" +
			"<walk>actor/human/movement/walk.xml</walk>" +
			"<run>actor/human/movement/walk.xml</run>" +
			"<attack>attack/weapon/sword.xml</attack>" +
			"<death>actor/human/death/death.xml</death>" +
		"</SoundGroups>" +
	"</a:example>" +
	"<element name='SoundGroups'>" +
		"<zeroOrMore>" + /* TODO: make this more specific, like a list of specific elements */
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
