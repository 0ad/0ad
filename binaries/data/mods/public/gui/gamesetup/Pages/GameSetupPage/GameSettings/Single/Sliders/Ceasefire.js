GameSettingControls.Ceasefire = class Ceasefire extends GameSettingControlSlider
{
	constructor(...args)
	{
		super(...args);

		this.sprintfValue = {};

		g_GameSettings.ceasefire.watch(() => this.render(), ["value"]);
		g_GameSettings.map.watch(() => this.render(), ["type"]);
		this.render();
	}

	render()
	{
		this.setEnabled(g_GameSettings.map.type != "scenario");

		let value = Math.round(g_GameSettings.ceasefire.value);
		this.sprintfValue.minutes = value;

		this.setSelectedValue(g_GameSettings.ceasefire.value,
			value == 0 ?
				this.NoCeasefireCaption :
				sprintf(this.CeasefireCaption(value), this.sprintfValue));
	}

	onValueChange(value)
	{
		g_GameSettings.ceasefire.setValue(value);
		this.gameSettingsControl.setNetworkInitAttributes();
	}
};

GameSettingControls.Ceasefire.prototype.TitleCaption =
	translate("Ceasefire");

GameSettingControls.Ceasefire.prototype.Tooltip =
	translate("Set time where no attacks are possible.");

GameSettingControls.Ceasefire.prototype.NoCeasefireCaption =
	translateWithContext("ceasefire", "No ceasefire");

GameSettingControls.Ceasefire.prototype.CeasefireCaption =
	minutes => translatePluralWithContext("ceasefire", "%(minutes)s minute", "%(minutes)s minutes", minutes);

GameSettingControls.Ceasefire.prototype.DefaultValue = 0;

GameSettingControls.Ceasefire.prototype.MinValue = 0;

GameSettingControls.Ceasefire.prototype.MaxValue = 45;
