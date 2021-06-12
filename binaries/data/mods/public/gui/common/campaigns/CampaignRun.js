// Cached run for CampaignRun.getCurrentRun()
// TODO: Move this to a static member once linters accept it.
var g_CurrentCampaignRun;

/**
 * A campaign "Run" saves metadata on a campaign progession.
 * It is equivalent to a saved game for a game.
 * It is named a "run" in an attempt to disambiguate with saved games from campaign runs,
 * campaign templates, and the actual concept of a campaign at large.
 *
 * The intent is that this file should be lightweight to load/save.
 */
class CampaignRun
{
	static getCurrentRunFilename()
	{
		return Engine.ConfigDB_GetValue("user", "currentcampaign");
	}

	static hasCurrentRun()
	{
		return !!CampaignRun.getCurrentRunFilename();
	}

	static getCurrentRun()
	{
		let current = CampaignRun.getCurrentRunFilename();
		if (g_CurrentCampaignRun && g_CurrentCampaignRun.ID == current)
			return g_CurrentCampaignRun.run;

		let run = new CampaignRun(current).load();
		g_CurrentCampaignRun = {
			"run": run,
			"ID": current
		};
		return run;
	}

	static clearCurrentRun()
	{
		Engine.ConfigDB_RemoveValue("user", "currentcampaign");
		Engine.ConfigDB_WriteFile("user", "config/user.cfg");
	}

	constructor(name = "")
	{
		this.filename = name;
		// Metadata on the run, such as its description.
		this.meta = {};
		// 'User' data
		this.data = {};
		// ID of the campaign templates.
		this.template = null;
	}

	setData(data)
	{
		if (!data)
		{
			warn("Invalid campaign scenario end data. Nothing will be saved.");
			return this;
		}

		this.data = data;
		this.save();
		return this;
	}

	setTemplate(template)
	{
		this.template = template;
		this.save();
		return this;
	}

	setMeta(description)
	{
		this.meta.userDescription = description;
		this.save();
		return this;
	}

	setCurrent()
	{
		Engine.ConfigDB_CreateValue("user", "currentcampaign", this.filename);
		Engine.ConfigDB_WriteValueToFile("user", "currentcampaign", this.filename, "config/user.cfg");
		g_CurrentCampaignRun = {
			"ID": this.filename,
			"run": this
		};
		return this;
	}

	isCurrent()
	{
		return this.filename === CampaignRun.getCurrentRunFilename();
	}

	getMenuPath()
	{
		return "campaigns/" + this.template.interface + "/page.xml";
	}

	getEndGamePath()
	{
		return "campaigns/" + this.template.interface + "/endgame/page.xml";
	}

	/**
	 * Return a readable name for this campaign.
	 * @param full - if true, include both user description and template name. Otherwise, skip one if they're identical.
	 */
	getLabel(full)
	{
		if (!full && this.meta.userDescription === translateWithContext("Campaign Template", this.template.Name))
			return this.meta.userDescription;
		return sprintf(translate("%(userDesc)s - %(templateName)s"), {
			"userDesc": this.meta.userDescription,
			"templateName": translateWithContext("Campaign Template", this.template.Name)
		});
	}

	load()
	{
		if (!Engine.FileExists("saves/campaigns/" + this.filename + ".0adcampaign"))
			throw new Error("Campaign file does not exist");
		let data = Engine.ReadJSONFile("saves/campaigns/" + this.filename + ".0adcampaign");
		this.data = data.data;
		this.meta = data.meta;
		this.template = CampaignTemplate.getTemplate(data.template_identifier);
		if (!this.template)
			throw new Error("Campaign template " + data.template_identifier + " does not exist (perhaps it comes from a mod?)");
		return this;
	}

	save()
	{
		let data = {
			"data": this.data,
			"meta": this.meta,
			"template_identifier": this.template.identifier
		};
		Engine.WriteJSONFile("saves/campaigns/" + this.filename + ".0adcampaign", data);
		return this;
	}

	destroy()
	{
		Engine.DeleteCampaignSave("saves/campaigns/" + this.filename + ".0adcampaign");
		if (CampaignRun.getCurrentRunFilename() === this.filename)
			CampaignRun.clearCurrentRun();
	}
}
