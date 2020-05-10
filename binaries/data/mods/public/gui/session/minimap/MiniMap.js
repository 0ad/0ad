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

	onWorldClick(target, button)
	{
		// Partly duplicated from handleInputAfterGui(), but with the input being
		// world coordinates instead of screen coordinates.
		if (button == SDL_BUTTON_LEFT)
		{
			if (inputState != INPUT_PRESELECTEDACTION || preSelectedAction == ACTION_NONE)
				return false;
		}
		else if (button == SDL_BUTTON_RIGHT)
		{
			if (inputState == INPUT_PRESELECTEDACTION)
			{
				preSelectedAction = ACTION_NONE;
				inputState = INPUT_NORMAL;
				return true;
			}
			else if (inputState != INPUT_NORMAL)
				return false;
		}
		else
			return false;


		if (!controlsPlayer(g_ViewedPlayer))
			return false;

		let action = determineAction(undefined, undefined, true);
		if (!action)
			return false;
		if (button == SDL_BUTTON_LEFT && !Engine.HotkeyIsPressed("session.queue") && !Engine.HotkeyIsPressed("session.orderone"))
		{
			preSelectedAction = ACTION_NONE;
			inputState = INPUT_NORMAL;
		}
		return handleUnitAction(target, action);
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
