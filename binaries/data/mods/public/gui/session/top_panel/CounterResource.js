/**
 * This class manages the counter in the top panel for one resource type.
 */
class CounterResource
{
	constructor(resCode, panel, icon, count, stats)
	{
		this.resCode = resCode;
		this.panel = panel;
		this.icon = icon;
		this.count = count;
		this.stats = stats;
	}

	rebuild(playerState, getAllyStatTooltip)
	{
		this.count.caption = abbreviateLargeNumbers(Math.floor(playerState.resourceCounts[this.resCode]));

		let gatherers = playerState.resourceGatherers[this.resCode];
		this.stats.caption = gatherers ? coloredText(gatherers, this.DefaultResourceGatherersColor) : 0;

		// TODO: Set the tooltip only if hovered?
		let description = g_ResourceData.GetResource(this.resCode).description;
		if (description)
			description = "\n" + translate(description);

		this.panel.tooltip =
			setStringTags(resourceNameFirstWord(this.resCode), CounterManager.ResourceTitleTags) +
			description +
			getAllyStatTooltip(this.getTooltipData.bind(this)) + "\n" + CounterPopulation.prototype.CurrentGatherersTooltip;
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

/**
 * Color to highlight the resource gatherers.
 */
CounterResource.prototype.DefaultResourceGatherersColor = "gold";
