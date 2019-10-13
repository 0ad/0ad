/**
 * This class manages a button of the trading goods selection,
 * where the user can determine the amounts of resource tpes that the trade carts shall transport.
 */
class TradeButton
{
	constructor(tradeButtonManager, resourceCode, i)
	{
		this.tradeButtonManager = tradeButtonManager;
		this.resourceCode = resourceCode;

		let id = "[" + i + "]";

		this.tradeArrowUp = Engine.GetGUIObjectByName("tradeArrowUp" + id);
		this.tradeArrowDn = Engine.GetGUIObjectByName("tradeArrowDn" + id);
		this.tradeResource = Engine.GetGUIObjectByName("tradeResource" + id);
		this.tradeResourceText = Engine.GetGUIObjectByName("tradeResourceText" + id);
		this.tradeResourceButton = Engine.GetGUIObjectByName("tradeResourceButton" + id);
		this.tradeResourceSelection = Engine.GetGUIObjectByName("tradeResourceSelection" + id);

		Engine.GetGUIObjectByName("tradeResourceIcon" + id).sprite =
			"stretched:" + this.ResourceIconPath + resourceCode + ".png";

		this.tradeResourceButton.onPress = () => { tradeButtonManager.selectResource(resourceCode); };

		this.tradeArrowUp.onPress = () => {
			tradeButtonManager.changeResourceAmount(resourceCode, +Math.min(this.AmountStep, tradeButtonManager.tradingGoods[tradeButtonManager.selectedResource]));
		};

		this.tradeArrowDn.onPress = () => {
			tradeButtonManager.changeResourceAmount(resourceCode, -Math.min(this.AmountStep, tradeButtonManager.tradingGoods[resourceCode]));
		};

		setPanelObjectPosition(this.tradeResource, i, i + 1);
	}

	update(enabled)
	{
		let isSelected = this.tradeButtonManager.selectedResource == this.resourceCode;
		let currentAmount = this.tradeButtonManager.tradingGoods[this.resourceCode];
		let selectedAmount = this.tradeButtonManager.tradingGoods[this.tradeButtonManager.selectedResource];

		this.tradeResourceText.caption = sprintf(translateWithContext("trading good ratio", this.AmountRatioCaption), {
			"amount": currentAmount
		});
		this.tradeResourceButton.enabled = enabled;
		this.tradeResourceSelection.hidden = !enabled || !isSelected;

		this.tradeArrowUp.enabled = enabled;
		this.tradeArrowDn.enabled = enabled;
		this.tradeArrowUp.hidden = !enabled || isSelected || currentAmount == 100 || selectedAmount == 0;
		this.tradeArrowDn.hidden = !enabled || isSelected || currentAmount == 0 || selectedAmount == 100;
	}
}

TradeButton.prototype.AmountRatioCaption = markForTranslationWithContext("trading good ratio", "%(amount)s%%");

TradeButton.getWidth = function()
{
	let size = Engine.GetGUIObjectByName("tradeResource[0]").size;
	return size.right - size.left;
};

TradeButton.prototype.ResourceIconPath = "session/icons/resources/";

/**
 * Percent of trading good selection to change per click.
 */
TradeButton.prototype.AmountStep = 5;
