GameSettingControls.Rating = class Rating extends GameSettingControlCheckbox
{
	constructor(...args)
	{
		super(...args);

		// The availability of rated games is not a GUI concern, unlike most other
		// potentially available settings.
		g_GameSettings.rating.watch(() => this.render(), ["enabled", "available"]);
		this.render();
	}

	render()
	{
		this.setHidden(!g_GameSettings.rating.available);
		this.setChecked(g_GameSettings.rating.enabled);
	}

	onPress(checked)
	{
		g_GameSettings.rating.setEnabled(checked);
		this.gameSettingsController.setNetworkInitAttributes();
	}
};

GameSettingControls.Rating.prototype.TitleCaption =
	translate("Rated Game");

GameSettingControls.Rating.prototype.Tooltip =
	translate("Toggle if this game will be rated for the leaderboard.");
