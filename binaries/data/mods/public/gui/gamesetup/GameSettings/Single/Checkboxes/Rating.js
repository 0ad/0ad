GameSettingControls.Rating = class extends GameSettingControlCheckbox
{
	constructor(...args)
	{
		super(...args);

		this.hasXmppClient = Engine.HasXmppClient();
		this.available = false;
	}

	onGameAttributesChange()
	{
		this.available = this.hasXmppClient && g_GameAttributes.settings.PlayerData.length == 2;
		if (this.available)
		{
			if (g_GameAttributes.settings.RatingEnabled === undefined)
			{
				g_GameAttributes.settings.RatingEnabled = true;
				this.gameSettingsControl.updateGameAttributes();
			}
		}
		else if (g_GameAttributes.settings.RatingEnabled !== undefined)
		{
			delete g_GameAttributes.settings.RatingEnabled;
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	onGameAttributesBatchChange()
	{
		this.setHidden(!this.available);
		if (this.available)
			this.setChecked(g_GameAttributes.settings.RatingEnabled);
	}

	onPress(checked)
	{
		g_GameAttributes.settings.RatingEnabled = checked;
		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();
	}

	onGameAttributesFinalize()
	{
		if (this.hasXmppClient)
			Engine.SetRankedGame(!!g_GameAttributes.settings.RatingEnabled);
	}
};

GameSettingControls.Rating.prototype.TitleCaption =
	translate("Rated Game");

GameSettingControls.Rating.prototype.Tooltip =
	translate("Toggle if this game will be rated for the leaderboard.");
