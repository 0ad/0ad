/**
 * This class manages the counter in the top panel for one resource type.
 */
class CounterResource
{
	constructor(resCode, panel, icon, count)
	{
		this.resCode = resCode;
		this.panel = panel;
		this.icon = icon;
		this.count = count;
	}

	rebuild(playerState, getAllyStatTooltip)
	{
		this.count.caption = Math.floor(playerState.resourceCounts[this.resCode]);

		// TODO: Set the tooltip only if hovered?
		let description = g_ResourceData.GetResource(this.resCode).description;
		if (description)
			description = "\n" + translate(description);

		this.panel.tooltip =
			setStringTags(resourceNameFirstWord(this.resCode), CounterManager.ResourceTitleTags) +
			description +
			getAllyStatTooltip(this.getTooltipData.bind(this));
	}

	getTooltipData(playerState, playername)
	{
		return {
			"playername": playername,
			"statValue": Math.round(playerState.resourceCounts[this.resCode]),
			"orderValue":  Math.round(playerState.resourceCounts[this.resCode])
		};
	}
}
