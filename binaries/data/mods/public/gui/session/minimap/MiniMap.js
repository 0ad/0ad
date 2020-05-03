/**
 * This class is concerned with handling events occurring when the interacts with the minimap,
 * except for changing the camera position on leftclick.
 */
class MiniMap
{
	constructor()
	{
		Engine.GetGUIObjectByName("minimap").onWorldClick = this.onWorldClick.bind(this);
		Engine.GetGUIObjectByName("minimap").onMouseEnter = this.onMouseEnter.bind(this);
		Engine.GetGUIObjectByName("minimap").onMouseLeave = this.onMouseLeave.bind(this);
		this.mouseIsOverMiniMap = false;
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

	onMouseEnter()
	{
		this.mouseIsOverMiniMap = true;
	}

	onMouseLeave()
	{
		this.mouseIsOverMiniMap = false;
	}

	isMouseOverMiniMap()
	{
		return this.mouseIsOverMiniMap;
	}
}
