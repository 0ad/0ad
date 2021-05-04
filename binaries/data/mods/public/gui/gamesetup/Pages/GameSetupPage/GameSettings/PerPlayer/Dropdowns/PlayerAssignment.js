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

		// Build the initial list of values with undefined & AI clients.
		this.rebuildList();

		g_GameSettings.playerAI.watch(() => this.render(), ["values"]);
		g_GameSettings.playerCount.watch((_, oldNb) => this.OnPlayerNbChange(oldNb), ["nbPlayers"]);
	}

	setControl()
	{
		this.dropdown = Engine.GetGUIObjectByName("playerAssignment[" + this.playerIndex + "]");
		this.label = Engine.GetGUIObjectByName("playerAssignmentText[" + this.playerIndex + "]");
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

	onPlayerAssignmentsChange()
	{
		// Rebuild the list to account for new/removed players.
		this.rebuildList();
		let newGUID;
		for (let guid in g_PlayerAssignments)
			if (g_PlayerAssignments[guid].player == this.playerIndex + 1)
			{
				newGUID = guid;
				break;
			}
		if (this.assignedGUID === newGUID)
			return;
		this.assignedGUID = newGUID;
		// Remove the AI from the slot if there was one.
		if (this.assignedGUID && g_GameSettings.playerAI.get(this.playerIndex))
		{
			g_GameSettings.playerAI.setAI(this.playerIndex, undefined);
			this.gameSettingsController.setNetworkInitAttributes();
		}
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
		// TODO: this particular bit is done for each row, which is unnecessarily inefficient.
		this.playerItems = sortGUIDsByPlayerID().map(
			this.clientItemFactory.createItem.bind(this.clientItemFactory));
		this.values = prepareForDropdown([
			...this.playerItems,
			...this.aiItems,
			this.unassignedItem
		]);

		let selected = this.dropdown.list_data?.[this.dropdown.selected];
		this.dropdown.list = this.values.Caption;
		this.dropdown.list_data = this.values.Value.map(x => x || "undefined");
		this.setSelectedValue(selected);
		Engine.ProfileStop();
	}

	onSelectionChange(itemIdx)
	{
		this.values.Handler[itemIdx].onSelectionChange(
			this.gameSettingsController,
			this.playerAssignmentsController,
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

		onSelectionChange(gameSettingsController, playerAssignmentsController, playerIndex, guidToAssign)
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

			playerAssignmentsController.assignPlayer(guidToAssign, playerIndex);
			gameSettingsController.setNetworkInitAttributes();
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

		onSelectionChange(gameSettingsController, playerAssignmentsController, playerIndex, value)
		{
			playerAssignmentsController.unassignClient(playerIndex + 1);

			g_GameSettings.playerAI.set(playerIndex, {
				"bot": value,
				"difficulty": +Engine.ConfigDB_GetValue("user", "gui.gamesetup.aidifficulty"),
				"behavior": Engine.ConfigDB_GetValue("user", "gui.gamesetup.aibehavior"),
			});

			gameSettingsController.setNetworkInitAttributes();
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

		onSelectionChange(gameSettingsController, playerAssignmentsController, playerIndex)
		{
			playerAssignmentsController.unassignClient(playerIndex + 1);

			g_GameSettings.playerAI.setAI(playerIndex, undefined);

			gameSettingsController.setNetworkInitAttributes();
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
