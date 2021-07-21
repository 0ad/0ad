PlayerSettingControls.PlayerColor = class PlayerColor extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		g_GameSettings.playerColor.watch(() => this.render(), ["values", "locked"]);
		this.render();
	}

	setControl()
	{
		this.dropdown = Engine.GetGUIObjectByName("playerColor[" + this.playerIndex + "]");
		this.playerBackgroundColor = Engine.GetGUIObjectByName("playerBackgroundColor[" + this.playerIndex + "]");
		this.playerColorHeading = Engine.GetGUIObjectByName("playerColorHeading");
	}

	render()
	{
		if (g_GameSettings.playerCount.nbPlayers < this.playerIndex + 1)
			return;

		const hidden = !g_IsController || g_GameSettings.map.type == "scenario";
		this.dropdown.hidden = hidden;
		this.playerColorHeading.hidden = hidden;

		const value = g_GameSettings.playerColor.get(this.playerIndex);
		this.playerBackgroundColor.sprite = "color:" + rgbToGuiColor(value, 100);

		this.values = g_GameSettings.playerColor.available;
		this.dropdown.list = this.values.map(color => coloredText(this.ColorIcon, rgbToGuiColor(color)));
		this.dropdown.list_data = this.values.map((color, i) => i);
		this.setSelectedValue(this.values.map((color, i) => {
			if (color.r === value.r && color.g === value.g && color.b === value.b)
				return i;
			return undefined;
		}).filter(x => x !== undefined)?.[0] ?? -1);
	}

	onSelectionChange(itemIdx)
	{
		g_GameSettings.playerColor.setColor(this.playerIndex, this.values[itemIdx]);
		this.gameSettingsController.setNetworkInitAttributes();
	}
};

PlayerSettingControls.PlayerColor.prototype.Tooltip =
	translate("Pick a color.");

PlayerSettingControls.PlayerColor.prototype.ColorIcon =
	"■";
