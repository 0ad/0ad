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
	}

	flare(target, playerID)
	{
		return this.miniMap.flare(target, playerID);
	}

	isMouseOverMiniMap()
	{
		return this.miniMap.isMouseOverMiniMap();
	}
}
