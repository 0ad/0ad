/**
 * This class updates the attack request button per player within the diplomacy dialog.
 * If an attack request is sent to Petra, the AI may respond with an attack or decline.
 */
DiplomacyDialogPlayerControl.prototype.AttackRequestButton = class
{
	constructor(playerID)
	{
		this.button = Engine.GetGUIObjectByName("diplomacyAttackRequest[" + (playerID - 1) + "]");
		this.button.tooltip = translate(this.Tooltip);
		this.button.onPress = this.onPress.bind(this);

		this.playerID = playerID;
	}

	update(playerInactive)
	{
		this.button.enabled = controlsPlayer(g_ViewedPlayer);
		this.button.hidden =
			playerInactive ||
			GetSimState().ceasefireActive ||
			!g_Players[g_ViewedPlayer].isEnemy[this.playerID] ||
			g_Players[g_ViewedPlayer].isMutualAlly.every(
				(isMutualAlly, playerID) => !isMutualAlly || playerID == g_ViewedPlayer);
	}

	onPress()
	{
		Engine.PostNetworkCommand({
			"type": "attack-request",
			"source": g_ViewedPlayer,
			"player": this.playerID
		});
	}
};

DiplomacyDialogPlayerControl.prototype.AttackRequestButton.prototype.Tooltip =
	markForTranslation("Request your allies to attack this enemy");
