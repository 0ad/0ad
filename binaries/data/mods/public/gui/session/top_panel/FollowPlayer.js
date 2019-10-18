/**
 * This class manages the checkbox that enables the observermode option to follow the commands of a player.
 */
class FollowPlayer
{
	constructor(playerViewControl)
	{
		this.viewPlayer = Engine.GetGUIObjectByName("viewPlayer");
		this.followPlayerLabel = Engine.GetGUIObjectByName("followPlayerLabel");
		this.optionFollowPlayer = Engine.GetGUIObjectByName("optionFollowPlayer");
		this.followPlayer = Engine.GetGUIObjectByName("followPlayer");

		this.followPlayer.onPress = this.onPress.bind(this);
		this.followPlayer.onWindowResized = this.onWindowResized.bind(this);
		playerViewControl.registerViewedPlayerChangeHandler(this.onViewedPlayerChange.bind(this));
	}

	onPress()
	{
		g_FollowPlayer = !g_FollowPlayer;
	}

	onViewedPlayerChange()
	{
		// Following gaia can be interesting on scripted maps
		this.optionFollowPlayer.hidden = !g_IsObserver || g_ViewedPlayer == -1;
	}

	onWindowResized()
	{
		this.followPlayerLabel.hidden =
			this.followPlayerLabel.getComputedSize().left + this.followPlayerLabel.getTextSize().width >
			this.viewPlayer.getComputedSize().left;
	}
}
