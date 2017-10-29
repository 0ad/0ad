function Sound() {}

Sound.prototype.Schema =
	"<a:help>Lists the sound groups associated with this unit.</a:help>" +
	"<a:example>" +
		"<SoundGroups>" +
			"<walk>actor/human/movement/walk.xml</walk>" +
			"<run>actor/human/movement/walk.xml</run>" +
			"<attack_melee>attack/weapon/sword.xml</attack_melee>" +
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

Sound.prototype.Serialize = null; // we have no dynamic state to save

Sound.prototype.GetSoundGroup = function(name)
{
	return this.template.SoundGroups[name] || "";
};

Sound.prototype.PlaySoundGroup = function(name)
{
	if (name in this.template.SoundGroups)
	{
		// Replace the "{lang}" codes with this entity's civ ID
		var cmpIdentity = Engine.QueryInterface(this.entity, IID_Identity);
		if (!cmpIdentity)
			return;
		var lang = cmpIdentity.GetLang();
		// Replace the "{gender}" codes with this entity's gender ID
		var gender = cmpIdentity.GetGender();

		var soundName = this.template.SoundGroups[name].replace(/\{lang\}/g, lang)
				.replace(/\{gender\}/g, gender);
		var cmpSoundManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_SoundManager);
		if (cmpSoundManager)
			cmpSoundManager.PlaySoundGroup(soundName, this.entity);
	}
};

Engine.RegisterComponentType(IID_Sound, "Sound", Sound);
