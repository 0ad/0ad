/**
 * This class controls the gamespeed.
 * The control is only available in single-player and replay mode.
 * Fast forwarding is enabled if and only if there is no human player assigned.
 */
class GameSpeedControl
{
	constructor(playerViewControl)
	{
		this.gameSpeed = Engine.GetGUIObjectByName("gameSpeed");
		this.gameSpeed.onSelectionChange = this.onSelectionChange.bind(this);

		registerPlayersInitHandler(this.rebuild.bind(this));
		registerPlayersFinishedHandler(this.rebuild.bind(this));
		playerViewControl.registerPlayerIDChangeHandler(this.rebuild.bind(this));
	}

	rebuild()
	{
		let player = g_Players[Engine.GetPlayerID()];

		let gameSpeeds = prepareForDropdown(g_Settings.GameSpeeds.filter(speed =>
			!speed.FastForward || !player || player.state != "active"));

		this.gameSpeed.list = gameSpeeds.Title;
		this.gameSpeed.list_data = gameSpeeds.Speed;

		let simRate = Engine.GetSimRate();

		// If the gamespeed is something like 0.100001 from the gamesetup, set it to 0.1
		let gameSpeedIdx = gameSpeeds.Speed.indexOf(+simRate.toFixed(2));
		this.gameSpeed.selected = gameSpeedIdx != -1 ? gameSpeedIdx : gameSpeeds.Default;
	}

	toggle()
	{
		this.gameSpeed.hidden = !this.gameSpeed.hidden;
	}

	onSelectionChange()
	{
		if (!g_IsNetworked)
			Engine.SetSimRate(+this.gameSpeed.list_data[this.gameSpeed.selected]);
	}
}
