PlayerSettingControls.PlayerFrame = class PlayerFrame extends GameSettingControl
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

		g_GameSettings.playerCount.watch(() => this.render(), ["nbPlayers"]);
		this.render();
	}

	render()
	{
		this.playerFrame.hidden = this.playerIndex >= g_GameSettings.playerCount.nbPlayers;
	}
};

PlayerSettingControls.PlayerFrame.prototype.Height = 32;
