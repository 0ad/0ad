PlayerSettingControls.PlayerFrame = class extends GameSettingControl
{
	constructor(...args)
	{
		super(...args);

		this.playerFrame = Engine.GetGUIObjectByName("playerFrame[" + this.playerIndex + "]");

		{
			let size = this.playerFrame.size;
			size.top = this.Height * this.playerIndex;
			size.bottom = this.Height * (this.playerIndex + 1);
			this.playerFrame.size = size;
		}
	}

	onGameAttributesBatchChange()
	{
		this.playerFrame.hidden = !this.gameSettingsControl.getPlayerData(g_GameAttributes, this.playerIndex);
	}
}

PlayerSettingControls.PlayerFrame.prototype.Height = 32;
