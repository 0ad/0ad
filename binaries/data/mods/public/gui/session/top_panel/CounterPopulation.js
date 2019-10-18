/**
 * This class manages the population counter in the top panel.
 * It flashes the counter if the training of any owned entity is blocked.
 */
class CounterPopulation
{
	constructor(resCode, panel, icon, count)
	{
		this.resCode = resCode;
		this.panel = panel;
		this.icon = icon;
		this.count = count;
		this.count.onTick = this.onTick.bind(this);
		this.isTrainingBlocked = false;
		this.color = this.DefaultPopulationColor;
	}

	rebuild(playerState, getAllyStatTooltip)
	{
		this.count.caption = sprintf(translate(this.CounterCaption), playerState);

		this.isTrainingBlocked = playerState.trainingBlocked;

		this.panel.tooltip =
			setStringTags(translate(this.PopulationTooltip), CounterManager.ResourceTitleTags) + "\n" +
			sprintf(translate(this.MaximumPopulationTooltip), { "popCap": playerState.popMax }) +
			getAllyStatTooltip(this.getTooltipData.bind(this));
	}

	getTooltipData(playerState, playername)
	{
		return {
			"playername": playername,
			"statValue": sprintf(translate(this.AllyPopulationTooltip), playerState),
			"orderValue": playerState.popCount
		};
	}

	onTick()
	{
		if (this.panel.hidden)
			return;

		let newColor = this.isTrainingBlocked && Date.now() % 1000 < 500 ?
			this.PopulationAlertColor :
			this.DefaultPopulationColor;

		if (newColor == this.color)
			return;

		this.color = newColor;
		this.count.textcolor = newColor;
	}
}

CounterPopulation.prototype.CounterCaption = markForTranslation("%(popCount)s/%(popLimit)s");

CounterPopulation.prototype.PopulationTooltip = markForTranslation("Population (current / limit)");

CounterPopulation.prototype.MaximumPopulationTooltip = markForTranslation("Maximum population: %(popCap)s");

CounterPopulation.prototype.AllyPopulationTooltip = markForTranslation("%(popCount)s/%(popLimit)s/%(popMax)s");

/**
 * Colors to flash when pop limit reached.
 */
CounterPopulation.prototype.DefaultPopulationColor = "white";
CounterPopulation.prototype.PopulationAlertColor = "orange";
