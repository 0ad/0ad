/**
 * This class is concerned with handling events occurring when the interacts with the minimap,
 * except for changing the camera position on leftclick.
 */
class MiniMap
{
	constructor()
	{
		Engine.GetGUIObjectByName("minimap").onWorldClick = this.onWorldClick.bind(this);
	}

	onWorldClick(target)
	{
		if (!controlsPlayer(g_ViewedPlayer))
			return false;

		// Partly duplicated from handleInputAfterGui(), but with the input being
		// world coordinates instead of screen coordinates.

		if (inputState != INPUT_NORMAL)
			return false;

		let action = determineAction(undefined, undefined, true);
		return action && handleUnitAction(target, action);
	}
}
