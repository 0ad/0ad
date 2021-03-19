/**
 * Store campaign-related data.
 * This is just a passthrough and makes no assumption about the data.
 */
GameSettings.prototype.Attributes.CampaignData = class CampaignData extends GameSetting
{
	init()
	{
	}

	toInitAttributes(attribs)
	{
		if (this.value)
			attribs.campaignData = this.value;
	}

	fromInitAttributes(attribs)
	{
		this.value = attribs.campaignData;
	}
};
