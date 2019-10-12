/**
 * This class provides one button per diplomatic stance for a given player.
 */
DiplomacyDialogPlayerControl.prototype.StanceButtonManager = class
{
	constructor(playerID)
	{
		this.buttons = this.Stances.map(stance =>
			new this.StanceButton(playerID, stance));
	}

	update(playerInactive)
	{
		let hidden = playerInactive || GetSimState().ceasefireActive || g_Players[g_ViewedPlayer].teamsLocked;

		for (let button of this.buttons)
			button.update(hidden);
	}
};

DiplomacyDialogPlayerControl.prototype.StanceButtonManager.prototype.Stances = ["Ally", "Neutral", "Enemy"];

/**
 * This class manages a button that if pressed, will change the diplomatic stance to the given player to the given stance.
 */
DiplomacyDialogPlayerControl.prototype.StanceButtonManager.prototype.StanceButton = class
{
	constructor(playerID, stance)
	{
		this.playerID = playerID;
		this.stance = stance;
		this.button = Engine.GetGUIObjectByName("diplomacyPlayer" + stance + "[" + (playerID - 1) + "]");
		this.button.onPress = this.onPress.bind(this);
	}

	update(hidden)
	{
		this.button.hidden = hidden;
		if (hidden)
			return;

		let isCurrentStance = g_Players[g_ViewedPlayer]["is" + this.stance][this.playerID];
		this.button.enabled = !isCurrentStance && controlsPlayer(g_ViewedPlayer);
		this.button.caption = isCurrentStance ?
			translateWithContext("diplomatic stance selection", this.StanceSelection) :
			"";
	}

	onPress()
	{
		Engine.PostNetworkCommand({
			"type": "diplomacy",
			"player": this.playerID,
			"to": this.stance.toLowerCase()
		});
	}
};

DiplomacyDialogPlayerControl.prototype.StanceButtonManager.prototype.StanceButton.prototype.StanceSelection =
	markForTranslationWithContext("diplomatic stance selection", "x");
