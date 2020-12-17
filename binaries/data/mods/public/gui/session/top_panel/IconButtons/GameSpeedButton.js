/**
 * This class handles the button that shows the game speed control.
 */
class GameSpeedButton
{
	constructor(gameSpeedControl)
	{
		let gameSpeedButton = Engine.GetGUIObjectByName("gameSpeedButton");
		gameSpeedButton.onPress = gameSpeedControl.toggle.bind(gameSpeedControl);
		gameSpeedButton.hidden = g_IsNetworked;
	}
}
