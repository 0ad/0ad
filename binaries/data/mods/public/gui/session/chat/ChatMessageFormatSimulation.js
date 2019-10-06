/**
 * These classes construct a chat message from simulation events initiated from the GuiInterface PushNotification method.
 */
class ChatMessageFormatSimulation
{
}

ChatMessageFormatSimulation.attack = class
{
	parse(msg)
	{
		if (msg.player != g_ViewedPlayer)
			return "";

		let message = msg.targetIsDomesticAnimal ?
			translate("Your livestock has been attacked by %(attacker)s!") :
			translate("You have been attacked by %(attacker)s!");

		return sprintf(message, {
			"attacker": colorizePlayernameByID(msg.attacker)
		});
	}
};

ChatMessageFormatSimulation.barter = class
{
	parse(msg)
	{
		if (!g_IsObserver || Engine.ConfigDB_GetValue("user", "gui.session.notifications.barter") != "true")
			return "";

		let amountsSold = {};
		amountsSold[msg.resourceSold] = msg.amountsSold;

		let amountsBought = {};
		amountsBought[msg.resourceBought] = msg.amountsBought;

		return sprintf(translate("%(player)s bartered %(amountsBought)s for %(amountsSold)s."), {
			"player": colorizePlayernameByID(msg.player),
			"amountsBought": getLocalizedResourceAmounts(amountsBought),
			"amountsSold": getLocalizedResourceAmounts(amountsSold)
		});
	}
};

ChatMessageFormatSimulation.diplomacy = class
{
	parse(msg)
	{
		let messageType;

		if (g_IsObserver)
			messageType = "observer";
		else if (Engine.GetPlayerID() == msg.sourcePlayer)
			messageType = "active";
		else if (Engine.GetPlayerID() == msg.targetPlayer)
			messageType = "passive";
		else
			return "";

		return sprintf(translate(this.strings[messageType][msg.status]), {
			"player": colorizePlayernameByID(messageType == "active" ? msg.targetPlayer : msg.sourcePlayer),
			"player2": colorizePlayernameByID(messageType == "active" ? msg.sourcePlayer : msg.targetPlayer)
		});
	}
};

ChatMessageFormatSimulation.diplomacy.prototype.strings = {
	"active": {
		"ally": markForTranslation("You are now allied with %(player)s."),
		"enemy": markForTranslation("You are now at war with %(player)s."),
		"neutral": markForTranslation("You are now neutral with %(player)s.")
	},
	"passive": {
		"ally": markForTranslation("%(player)s is now allied with you."),
		"enemy": markForTranslation("%(player)s is now at war with you."),
		"neutral": markForTranslation("%(player)s is now neutral with you.")
	},
	"observer": {
		"ally": markForTranslation("%(player)s is now allied with %(player2)s."),
		"enemy": markForTranslation("%(player)s is now at war with %(player2)s."),
		"neutral": markForTranslation("%(player)s is now neutral with %(player2)s.")
	}
};

ChatMessageFormatSimulation.phase = class
{
	parse(msg)
	{
		let notifyPhase = Engine.ConfigDB_GetValue("user", "gui.session.notifications.phase");
		if (notifyPhase == "none" || msg.player != g_ViewedPlayer && !g_IsObserver && !g_Players[msg.player].isMutualAlly[g_ViewedPlayer])
			return "";

		let message = "";
		if (notifyPhase == "all")
		{
			if (msg.phaseState == "started")
				message = translate("%(player)s is advancing to the %(phaseName)s.");
			else if (msg.phaseState == "aborted")
				message = translate("The %(phaseName)s of %(player)s has been aborted.");
		}
		if (msg.phaseState == "completed")
			message = translate("%(player)s has reached the %(phaseName)s.");

		return sprintf(message, {
			"player": colorizePlayernameByID(msg.player),
			"phaseName": getEntityNames(GetTechnologyData(msg.phaseName, g_Players[msg.player].civ))
		});
	}
};

ChatMessageFormatSimulation.playerstate = class
{
	parse(msg)
	{
		if (!msg.message.pluralMessage)
			return sprintf(translate(msg.message), {
				"player": colorizePlayernameByID(msg.players[0])
			});

		let mPlayers = msg.players.map(playerID => colorizePlayernameByID(playerID));
		let lastPlayer = mPlayers.pop();

		return sprintf(translatePlural(msg.message.message, msg.message.pluralMessage, msg.message.pluralCount), {
			// Translation: This comma is used for separating first to penultimate elements in an enumeration.
			"players": mPlayers.join(translate(", ")),
			"lastPlayer": lastPlayer
		});
	}
};

/**
 * Optionally show all tributes sent in observer mode and tributes sent between allied players.
 * Otherwise, only show tributes sent directly to us, and tributes that we send.
 */
ChatMessageFormatSimulation.tribute = class
{
	parse(msg)
	{
		let message = "";
		if (msg.targetPlayer == Engine.GetPlayerID())
			message = translate("%(player)s has sent you %(amounts)s.");
		else if (msg.sourcePlayer == Engine.GetPlayerID())
			message = translate("You have sent %(player2)s %(amounts)s.");
		else if (Engine.ConfigDB_GetValue("user", "gui.session.notifications.tribute") == "true" &&
		        (g_IsObserver || g_GameAttributes.settings.LockTeams &&
		           g_Players[msg.sourcePlayer].isMutualAlly[Engine.GetPlayerID()] &&
		           g_Players[msg.targetPlayer].isMutualAlly[Engine.GetPlayerID()]))
			message = translate("%(player)s has sent %(player2)s %(amounts)s.");

		return sprintf(message, {
			"player": colorizePlayernameByID(msg.sourcePlayer),
			"player2": colorizePlayernameByID(msg.targetPlayer),
			"amounts": getLocalizedResourceAmounts(msg.amounts)
		});
	}
};
