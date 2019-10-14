/**
 * This class manages a buy and sell button for one resource.
 * The sell button selects the resource to be sold.
 * The buy button selects the resource to be bought and performs the sale.
 */
class BarterButton
{
	constructor(barterButtonManager, resourceCode, i, panel)
	{
		this.barterButtonManager = barterButtonManager;
		this.resourceCode = resourceCode;
		this.amountToSell = 0;

		this.sellButton = panel.children[i].children[0].children[0];
		this.sellIcon = this.sellButton.children[0];
		this.sellAmount = this.sellButton.children[1];
		this.sellSelection = this.sellButton.children[2];

		this.buyButton = panel.children[i].children[0].children[1];
		this.buyIcon= this.buyButton.children[0];
		this.buyAmount = this.buyButton.children[1];

		let resourceName = { "resource": resourceNameWithinSentence(resourceCode) };

		this.sellButton.tooltip = sprintf(this.SellTooltip, resourceName);
		this.sellButton.onPress = () => { barterButtonManager.setSelectedResource(this.resourceCode); };
		this.sellButton.hidden = false;

		this.buyButton.tooltip = sprintf(this.BuyTooltip, resourceName);
		this.buyButton.onPress = () => { this.buy(); };

		this.iconPath = this.ResourceIconPath + resourceCode + ".png";

		setPanelObjectPosition(panel.children[i], i, Infinity);
	}

	buy()
	{
		Engine.PostNetworkCommand({
			"type": "barter",
			"sell": this.barterButtonManager.selectedResource,
			"buy": this.resourceCode,
			"amount": this.amountToSell
		});
	}

	/**
	 * The viewed player might be the owner of the selected market.
	 */
	update(viewedPlayer)
	{
		this.amountToSell = this.BarterResourceSellQuantity;

		if (Engine.HotkeyIsPressed("session.massbarter"))
			this.amountToSell *= this.Multiplier;

		let neededResourcesSell = Engine.GuiInterfaceCall("GetNeededResources", {
			"cost": {
				[this.resourceCode]: this.amountToSell
			},
			"player": viewedPlayer
		});

		let neededResourcesBuy = Engine.GuiInterfaceCall("GetNeededResources", {
			"cost": {
				[this.barterButtonManager.selectedResource]: this.amountToSell
			},
			"player": viewedPlayer
		});

		let isSelected = this.resourceCode == this.barterButtonManager.selectedResource;
		let icon = "stretched:" +  (isSelected ? this.SelectedModifier : "") + this.iconPath;
		this.sellIcon.sprite = (neededResourcesSell ? this.DisabledModifier : "") + icon;
		this.buyIcon.sprite = (neededResourcesBuy ? this.DisabledModifier : "") + icon;

		this.sellAmount.caption = sprintf(translateWithContext("sell action", this.SellCaption), {
			"amount": this.amountToSell
		});

		let prices = GetSimState().players[viewedPlayer].barterPrices;

		this.buyAmount.caption = sprintf(translateWithContext("buy action", this.BuyCaption), {
			"amount": Math.round(
				prices.sell[this.barterButtonManager.selectedResource] /
				prices.buy[this.resourceCode] * this.amountToSell)
		});

		this.buyButton.hidden = isSelected;
		this.buyButton.enabled = controlsPlayer(viewedPlayer);
		this.sellSelection.hidden = !isSelected;
	}
}

BarterButton.getWidth = function(panel)
{
	let size = panel.children[0].size;
	return size.right - size.left;
};

BarterButton.prototype.BuyTooltip = markForTranslation("Buy %(resource)s");
BarterButton.prototype.SellTooltip = markForTranslation("Sell %(resource)s");
BarterButton.prototype.BuyCaption = markForTranslationWithContext("buy action", "+%(amount)s");
BarterButton.prototype.SellCaption = markForTranslationWithContext("sell action", "-%(amount)s");

/**
 * The barter constants should match with the simulation Quantity of goods to sell per click.
 */
BarterButton.prototype.BarterResourceSellQuantity = 100;

/**
 * Multiplier to be applied when holding the massbarter hotkey.
 */
BarterButton.prototype.Multiplier = 5;

BarterButton.prototype.SelectedModifier = "color:0 0 0 100:grayscale:";

BarterButton.prototype.DisabledModifier = "color:255 0 0 80:";

BarterButton.prototype.ResourceIconPath = "session/icons/resources/";
