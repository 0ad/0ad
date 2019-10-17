/**
 * This class displays the version information in the top panel.
 */
class BuildLabel
{
	constructor(playerViewControl)
	{
		this.viewPlayer = Engine.GetGUIObjectByName("viewPlayer");
		this.buildLabel = Engine.GetGUIObjectByName("buildLabel");

		Engine.GetGUIObjectByName("buildTimeLabel").caption = getBuildString();

		playerViewControl.registerViewedPlayerChangeHandler(this.onViewedPlayerChanged.bind(this));
	}

	onViewedPlayerChanged()
	{
		let isPlayer = g_ViewedPlayer > 0;
		this.buildLabel.hidden = isPlayer && !this.viewPlayer.hidden;
		this.buildLabel.size = isPlayer ? this.SizePlayer : this.SizeObserver;
	}
}

BuildLabel.prototype.SizePlayer = "50%+44 0 100%-283 100%";

BuildLabel.prototype.SizeObserver = "155 0 85%-279 100%";
