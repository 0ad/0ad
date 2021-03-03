// TODO: Move this to a static member once linters accept it.
var g_CachedTemplates;

class CampaignTemplate
{
	/**
	 * @returns a dictionary of campaign templates, as [ { 'identifier': id, 'data': data }, ... ]
	 */
	static getAvailableTemplates()
	{
		if (g_CachedTemplates)
			return g_CachedTemplates;

		let campaigns = Engine.ListDirectoryFiles("campaigns/", "*.json", false);

		g_CachedTemplates = [];

		for (let filename of campaigns)
			// Use file name as identifier to guarantee unicity.
			g_CachedTemplates.push(new CampaignTemplate(filename.slice("campaigns/".length, -".json".length)));

		return g_CachedTemplates;
	}

	static getTemplate(identifier)
	{
		if (!g_CachedTemplates)
			CampaignTemplate.getAvailableTemplates();
		let temp = g_CachedTemplates.filter(t => t.identifier == identifier);
		if (!temp.length)
			return null;
		return temp[0];
	}

	constructor(identifier)
	{
		Object.assign(this, Engine.ReadJSONFile("campaigns/" + identifier + ".json"));

		this.identifier = identifier;

		if (this.Interface)
			this.interface = this.Interface;
		else
			this.interface = "default_menu";

		if (!this.isValid())
			throw ("Campaign template " + this.identifier + ".json is not a valid campaign template.");
	}

	isValid()
	{
		return this.Name;
	}
}
