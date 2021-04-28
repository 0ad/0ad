// TODO: There should be an indication which player is not ready yet
// The color does not indicate it's meaning and is insufficient to inform many players.
PlayerSettingControls.PlayerName = class PlayerName extends GameSettingControl
{
	constructor(...args)
	{
		super(...args);

		this.playerName = Engine.GetGUIObjectByName("playerName[" + this.playerIndex + "]");
		g_GameSettings.playerCount.watch(() => this.render(), ["nbPlayers"]);

		this.guid = undefined;
		this.render();
	}

	onPlayerAssignmentsChange()
	{
		this.guid = undefined;

		for (let guid in g_PlayerAssignments)
			if (g_PlayerAssignments[guid].player == this.playerIndex + 1)
			{
				this.guid = guid;
				break;
			}

		this.render();
	}

	render()
	{
		let name = this.guid ? g_PlayerAssignments[this.guid].name :
			g_GameSettings.playerName.values[this.playerIndex];

		if (g_IsNetworked)
		{
			let status = this.guid ? g_PlayerAssignments[this.guid].status : this.ReadyTags.length - 1;
			name = setStringTags(name, this.ReadyTags[status]);
		}
		else if (name && !this.guid)
		{
			name = translate(name);
		}

		this.playerName.caption = name;
	}
};

PlayerSettingControls.PlayerName.prototype.ReadyTags = [
	{
		"color": "white",
	},
	{
		"color": "green",
	},
	{
		"color": "150 150 250",
	}
];
