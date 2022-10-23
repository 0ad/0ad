/**
 * This class is concerned with managing the different elements of the minimap panel.
 */
class MiniMapPanel
{
	constructor(playerViewControl, diplomacyColors, idleWorkerClasses)
	{
		this.diplomacyColorsButton = new MiniMapDiplomacyColorsButton(diplomacyColors);
		this.idleWorkerButton = new MiniMapIdleWorkerButton(playerViewControl, idleWorkerClasses);
		this.flareButton = new MiniMapFlareButton(playerViewControl);
		this.miniMap = new MiniMap();
		playerViewControl.registerViewedPlayerChangeHandler(this.rebuild.bind(this));
		registerHotkeyChangeHandler(this.rebuild.bind(this));
	}

	flare(target, playerID)
	{
		return this.miniMap.flare(target, playerID);
	}

	isMouseOverMiniMap()
	{
		return this.miniMap.isMouseOverMiniMap();
	}

	rebuild()
	{
		this.setCivBackgroundTexture();
	}

	setCivBackgroundTexture()
	{
		const playerCiv = g_ViewedPlayer > 0 ? g_Players[g_ViewedPlayer].civ : "gaia";
		const backgroundObject = Engine.GetGUIObjectByName("minimapBackgroundTexture");
		backgroundObject.sprite = `stretched:session/icons/bkg/background_circle_${playerCiv}.png`;
	}
}
