// Declare this first to avoid redundant lint warnings.
class PlayerAssignmentItem
{
}

/**
 * Warning: this class handles more than most other GUI controls.
 * Indeed, the logic of how to handle player assignments is here,
 * as that is not really a GUI-agnostic concern
 * (campaigns and other autostarting scripts should handle that themselves).
 */
PlayerSettingControls.PlayerAssignment = class PlayerAssignment extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		this.clientItemFactory = new PlayerAssignmentItem.Client();
		this.aiItemFactory = new PlayerAssignmentItem.AI();
		this.unassignedItem = new PlayerAssignmentItem.Unassigned().createItem();

		this.aiItems =
			g_Settings.AIDescriptions.filter(ai => !ai.data.hidden).map(
				this.aiItemFactory.createItem.bind(this.aiItemFactory));

		this.values = undefined;
		this.assignedGUID = undefined;
		this.fixedAI = undefined;

		g_GameSettings.playerAI.watch(() => this.render(), ["values"]);
		g_GameSettings.playerCount.watch((_, oldNb) => this.OnPlayerNbChange(oldNb), ["nbPlayers"]);

		// Sets up the dropdown and renders.
		this.onPlayerAssignmentsChange();
		this.playerAssignmentsControl.registerClientJoinHandler(this.onClientJoin.bind(this));
	}

	setControl()
	{
		this.dropdown = Engine.GetGUIObjectByName("playerAssignment[" + this.playerIndex + "]");
		this.label = Engine.GetGUIObjectByName("playerAssignmentText[" + this.playerIndex + "]");
	}

	onLoad(initData, hotloadData)
	{
		if (!hotloadData && !g_IsNetworked)
			this.onClientJoin("local", g_PlayerAssignments);
		this.playerAssignmentsControl.updatePlayerAssignments();
	}

	OnPlayerNbChange(oldNb)
	{
		let isPlayerSlot = Object.values(g_PlayerAssignments).some(x => x.player === this.playerIndex + 1);
		if (!isPlayerSlot && !g_GameSettings.playerAI.get(this.playerIndex) &&
			this.playerIndex >= oldNb && this.playerIndex < g_GameSettings.playerCount.nbPlayers)
		{
			// Add AIs to unused slots by default.
			// TODO: we could save the settings in case the player lowers, then re-raises the # of players.
			g_GameSettings.playerAI.set(this.playerIndex, {
				"bot": g_Settings.PlayerDefaults[this.playerIndex + 1].AI,
				"difficulty": +Engine.ConfigDB_GetValue("user", "gui.gamesetup.aidifficulty"),
				"behavior": Engine.ConfigDB_GetValue("user", "gui.gamesetup.aibehavior"),
			});
		}
	}

	onClientJoin(newGUID, newAssignments)
	{
		if (!g_IsController || this.fixedAI || newAssignments[newGUID].player != -1)
			return;

		// Assign the client (or only buddies if prefered) to a free slot
		if (newGUID != Engine.GetPlayerGUID())
		{
			let assignOption = Engine.ConfigDB_GetValue("user", this.ConfigAssignPlayers);
			if (assignOption == "disabled" ||
				assignOption == "buddies" && g_Buddies.indexOf(splitRatingFromNick(newAssignments[newGUID].name).nick) == -1)
				return;
		}

		for (let guid in newAssignments)
			if (newAssignments[guid].player == this.playerIndex + 1)
				return;

		newAssignments[newGUID].player = this.playerIndex + 1;
		this.playerAssignmentsControl.assignClient(newGUID, this.playerIndex + 1);
	}

	onPlayerAssignmentsChange()
	{
		let newGUID;
		for (let guid in g_PlayerAssignments)
			if (g_PlayerAssignments[guid].player == this.playerIndex + 1)
			{
				newGUID = guid;
				break;
			}
		this.assignedGUID = newGUID;
		this.rebuildList();
		this.render();
	}

	render()
	{
		this.setEnabled(true);
		if (this.assignedGUID)
		{
			this.setSelectedValue(this.assignedGUID);
			return;
		}
		let ai = g_GameSettings.playerAI.get(this.playerIndex);
		if (ai)
		{
			this.setSelectedValue(ai.bot);
			return;
		}

		this.setSelectedValue(undefined);
	}

	rebuildList()
	{
		Engine.ProfileStart("updatePlayerAssignmentsList");
		this.playerItems = sortGUIDsByPlayerID().map(
			this.clientItemFactory.createItem.bind(this.clientItemFactory));
		this.values = prepareForDropdown([
			...this.playerItems,
			...this.aiItems,
			this.unassignedItem
		]);

		this.dropdown.list = this.values.Caption;
		this.dropdown.list_data = this.values.Value;
		Engine.ProfileStop();
	}

	onSelectionChange(itemIdx)
	{
		this.values.Handler[itemIdx].onSelectionChange(
			this.gameSettingsControl,
			this.playerAssignmentsControl,
			this.playerIndex,
			this.values.Value[itemIdx]);
	}

	getAutocompleteEntries()
	{
		return this.values.Autocomplete;
	}
};

PlayerSettingControls.PlayerAssignment.prototype.Tooltip =
	translate("Select player.");

PlayerSettingControls.PlayerAssignment.prototype.AutocompleteOrder = 100;

PlayerSettingControls.PlayerAssignment.prototype.ConfigAssignPlayers =
	"gui.gamesetup.assignplayers";

{
	PlayerAssignmentItem.Client = class
	{
		createItem(guid)
		{
			return {
				"Handler": this,
				"Value": guid,
				"Autocomplete": g_PlayerAssignments[guid].name,
				"Caption": setStringTags(
					g_PlayerAssignments[guid].name,
					g_PlayerAssignments[guid].player == -1 ? this.ObserverTags : this.PlayerTags)
			};
		}

		onSelectionChange(gameSettingsControl, playerAssignmentsControl, playerIndex, guidToAssign)
		{
			let sourcePlayer = g_PlayerAssignments[guidToAssign].player - 1;
			if (sourcePlayer >= 0)
			{
				let ai = g_GameSettings.playerAI.get(playerIndex);
				// If the target was an AI, swap so AI settings are kept.
				if (ai)
					g_GameSettings.playerAI.swap(sourcePlayer, playerIndex);
				// Swap color + civ as well - this allows easy reorganizing of player order.
				if (g_GameSettings.map.type !== "scenario")
				{
					g_GameSettings.playerCiv.swap(sourcePlayer, playerIndex);
					g_GameSettings.playerColor.swap(sourcePlayer, playerIndex);
				}
			}

			playerAssignmentsControl.assignPlayer(guidToAssign, playerIndex);
			gameSettingsControl.setNetworkInitAttributes();
		}

		isSelected(pData, guid, value)
		{
			return guid !== undefined && guid == value;
		}
	};

	PlayerAssignmentItem.Client.prototype.PlayerTags =
		{ "color": "white" };

	PlayerAssignmentItem.Client.prototype.ObserverTags =
		{ "color": "170 170 250" };
}

{
	PlayerAssignmentItem.AI = class
	{
		createItem(ai)
		{
			let aiName = translate(ai.data.name);
			return {
				"Handler": this,
				"Value": ai.id,
				"Autocomplete": aiName,
				"Caption": setStringTags(sprintf(this.Label, { "ai": aiName }), this.Tags)
			};
		}

		onSelectionChange(gameSettingsControl, playerAssignmentsControl, playerIndex, value)
		{
			playerAssignmentsControl.unassignClient(playerIndex + 1);

			g_GameSettings.playerAI.set(playerIndex, {
				"bot": value,
				"difficulty": +Engine.ConfigDB_GetValue("user", "gui.gamesetup.aidifficulty"),
				"behavior": Engine.ConfigDB_GetValue("user", "gui.gamesetup.aibehavior"),
			});

			gameSettingsControl.setNetworkInitAttributes();
		}

		isSelected(pData, guid, value)
		{
			return !guid && pData.AI && pData.AI == value;
		}
	};

	PlayerAssignmentItem.AI.prototype.Label =
		translate("AI: %(ai)s");

	PlayerAssignmentItem.AI.prototype.Tags =
		{ "color": "70 150 70" };
}

{
	PlayerAssignmentItem.Unassigned = class
	{
		createItem()
		{
			return {
				"Handler": this,
				"Value": undefined,
				"Autocomplete": this.Label,
				"Caption": setStringTags(this.Label, this.Tags)
			};
		}

		onSelectionChange(gameSettingsControl, playerAssignmentsControl, playerIndex)
		{
			playerAssignmentsControl.unassignClient(playerIndex + 1);

			g_GameSettings.playerAI.setAI(playerIndex, undefined);

			gameSettingsControl.setNetworkInitAttributes();
		}

		isSelected(pData, guid, value)
		{
			return !guid && !pData.AI;
		}
	};

	PlayerAssignmentItem.Unassigned.prototype.Label =
		translate("Unassigned");

	PlayerAssignmentItem.Unassigned.prototype.Tags =
		{ "color": "140 140 140" };
}
