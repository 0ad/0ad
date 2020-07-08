/**
 * This class contains code common to the Structure Tree, Template Viewer, and any other "Reference Page" that may be added in the future.
 */
class ReferencePage
{
	constructor()
	{
		this.civData = loadCivData(true, false);

		this.TemplateLoader = new TemplateLoader();
		this.TemplateLister = new TemplateLister(this.TemplateLoader);
		this.TemplateParser = new TemplateParser(this.TemplateLoader);

		this.activeCiv = this.TemplateLoader.DefaultCiv;

		this.currentTemplateLists = {};
	}

	setActiveCiv(civCode)
	{
		if (civCode == this.TemplateLoader.DefaultCiv)
			return;

		this.activeCiv = civCode;

		this.currentTemplateLists = this.TemplateLister.compileTemplateLists(this.activeCiv, this.civData);
		this.TemplateParser.deriveModifications(this.activeCiv);
		this.TemplateParser.derivePhaseList(this.currentTemplateLists.techs.keys(), this.activeCiv);
	}

	/**
	 * Concatanates the return values of the array of passed functions.
	 *
	 * @param {object} template
	 * @param {array} textFunctions
	 * @param {string} joiner
	 * @return {string} The built text.
	 */
	static buildText(template, textFunctions=[], joiner="\n")
	{
		return textFunctions.map(func => func(template)).filter(tip => tip).join(joiner);
	}
}

ReferencePage.prototype.IconPath = "session/portraits/";

/**
 * List of functions that get the statistics of any template or entity,
 * formatted in such a way as to appear in a tooltip.
 *
 * The functions listed are defined in gui/common/tooltips.js
 */
ReferencePage.prototype.StatsFunctions = [
	getHealthTooltip,
	getHealerTooltip,
	getAttackTooltip,
	getSplashDamageTooltip,
	getArmorTooltip,
	getGarrisonTooltip,
	getProjectilesTooltip,
	getSpeedTooltip,
	getGatherTooltip,
	getResourceSupplyTooltip,
	getPopulationBonusTooltip,
	getResourceTrickleTooltip,
	getLootTooltip
];
