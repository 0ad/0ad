/**
 * This class composes the texts summarizing the current status of trader activity.
 */
class TraderStatusText
{
	constructor()
	{
		this.traderCountText = Engine.GetGUIObjectByName("traderCountText");

		this.components = Object.keys(this.Components.prototype).map(name =>
			new this.Components.prototype[name]());
	}

	update()
	{
		let traderNumber = Engine.GuiInterfaceCall("GetTraderNumber", g_ViewedPlayer);
		this.traderCountText.caption = this.components.reduce((caption, component) =>
			caption += component.getText(traderNumber, this.IdleTraderTextTags) + "\n\n", "").trim();
	}
}

TraderStatusText.prototype.IdleTraderTextTags = { "color": "orange" };

/**
 * This class stores classes that build a trader information text and can be extended in externally.
 */
TraderStatusText.prototype.Components = class
{
};
