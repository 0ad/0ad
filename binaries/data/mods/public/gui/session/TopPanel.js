/**
 * This class owns all classes that are part of the top panel.
 */
class TopPanel
{
	constructor(playerViewControl, diplomacyDialog, tradeDialog, objectivesDialog, gameSpeedControl)
	{
		this.counterManager = new CounterManager(playerViewControl);
		this.civIcon = new CivIcon(playerViewControl);
		this.buildLabel = new BuildLabel(playerViewControl);

		this.followPlayer = new FollowPlayer(playerViewControl);

		this.diplomacyDialogButton = new DiplomacyDialogButton(playerViewControl, diplomacyDialog);
		this.gameSpeedButton = new GameSpeedButton(gameSpeedControl);
		this.objectivesDialogButton = new ObjectivesDialogButton(objectivesDialog);
		this.tradeDialogButton = new TradeDialogButton(playerViewControl, tradeDialog);
	}
}
