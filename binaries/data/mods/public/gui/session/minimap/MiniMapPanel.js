/**
 * This class is concerned with managing the different elements of the minimap panel.
 */
class MiniMapPanel
{
	constructor(diplomacyColors, idleWorkerClasses)
	{
		this.diplomacyColorsButton = new MiniMapDiplomacyColorsButton(diplomacyColors);
		this.idleWorkerButton = new MiniMapIdleWorkerButton(idleWorkerClasses);
		this.minimap = new Minimap();
	}

	update()
	{
		this.diplomacyColorsButton.update();
		this.idleWorkerButton.update();
	}
}
