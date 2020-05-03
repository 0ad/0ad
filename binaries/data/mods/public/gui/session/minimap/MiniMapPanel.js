/**
 * This class is concerned with managing the different elements of the minimap panel.
 */
class MiniMapPanel
{
	constructor(playerViewControl, diplomacyColors, idleWorkerClasses)
	{
		this.diplomacyColorsButton = new MiniMapDiplomacyColorsButton(diplomacyColors);
		this.idleWorkerButton = new MiniMapIdleWorkerButton(playerViewControl, idleWorkerClasses);
		this.miniMap = new MiniMap();
	}

	isMouseOverMiniMap()
	{
		return this.miniMap.isMouseOverMiniMap();
	}
}
