/**
 * The GameSettingControl is an abstract class that is inherited by gamesetting control classes specific to a GUI object type,
 * such as the GameSettingControlCheckbox or GameSettingControlDropdown.
 *
 * These classes are abstract classes too and are implemented by each handler class specific to one logical setting of g_GameAttributes.
 * The purpose of these classes is to control precisely one logical setting of g_GameAttributes.
 * Having one class per logical setting allows to handle each setting without making a restriction as to how the property should be written to g_GameAttributes or g_PlayerAssignments.
 * The base classes allow implementing that while avoiding duplication.
 *
 * A GameSettingControl may depend on and read from other g_GameAttribute values,
 * but the class instance is to be the sole instance writing to its setting value in g_GameAttributes and
 * shall not write to setting values of other logical settings.
 *
 * The derived classes shall not make assumptions on the validity of g_GameAttributes,
 * sanitize or delete their value if it is incompatible.
 *
 * A class should only write values to g_GameAttributes that it itself has confirmed to be accurate.
 * This means that handlers may not copy an entire object or array of values, for example on mapchange.
 * This avoids writing a setting value to g_GameAttributes that is not tracked and deleted when it becomes invalid.
 *
 * Since GameSettingControls shall be able to subscribe to g_GameAttributes changes,
 * it is an obligation of the derived GameSettingControl class to broadcast the GameAttributesChange event each time it changes g_GameAttributes.
 */
class GameSettingControl
{
	// The constructor and inherited constructors shall not modify game attributes,
	// since all GameSettingControl shall be able to subscribe to any gamesetting change.
	constructor(gameSettingControlManager, category, playerIndex, setupWindow)
	{
		// Store arguments
		{
			this.category = category;
			this.playerIndex = playerIndex;

			this.setupWindow = setupWindow;
			this.gameSettingsControl = setupWindow.controls.gameSettingsControl;
			this.mapCache = setupWindow.controls.mapCache;
			this.mapFilters = setupWindow.controls.mapFilters;
			this.netMessages = setupWindow.controls.netMessages;
			this.playerAssignmentsControl = setupWindow.controls.playerAssignmentsControl;
		}

		// enabled and hidden should only be modified through their setters or
		// by calling updateVisibility after modification.
		this.enabled = true;
		this.hidden = false;

		if (this.setControl)
			this.setControl(gameSettingControlManager);

		// This variable also used for autocompleting chat.
		this.autocompleteTitle = undefined;

		if (this.title && this.TitleCaption)
			this.setTitle(this.TitleCaption);

		if (this.Tooltip)
			this.setTooltip(this.Tooltip);

		this.setHidden(false);

		if (this.onMapChange)
			this.gameSettingsControl.registerMapChangeHandler(this.onMapChange.bind(this));

		if (this.onLoad)
			this.setupWindow.registerLoadHandler(this.onLoad.bind(this));

		if (this.onGameAttributesChange)
			this.gameSettingsControl.registerGameAttributesChangeHandler(this.onGameAttributesChange.bind(this));

		if (this.onGameAttributesBatchChange)
			this.gameSettingsControl.registerGameAttributesBatchChangeHandler(this.onGameAttributesBatchChange.bind(this));

		if (this.onAssignPlayer && this.playerIndex === 0)
			this.gameSettingsControl.registerAssignPlayerHandler(this.onAssignPlayer.bind(this));

		if (this.onPickRandomItems)
			this.gameSettingsControl.registerPickRandomItemsHandler(this.onPickRandomItems.bind(this));

		if (this.onGameAttributesFinalize)
			this.gameSettingsControl.registerGameAttributesFinalizeHandler(this.onGameAttributesFinalize.bind(this));

		if (this.onPlayerAssignmentsChange)
			this.playerAssignmentsControl.registerPlayerAssignmentsChangeHandler(this.onPlayerAssignmentsChange.bind(this));
	}

	setTitle(titleCaption)
	{
		this.autocompleteTitle = titleCaption;
		this.title.caption = sprintf(this.TitleCaptionFormat, {
			"setting": titleCaption
		});
	}

	setTooltip(tooltip)
	{
		if (this.title)
			this.title.tooltip = tooltip;

		if (this.label)
			this.label.tooltip = tooltip;

		if (this.setControlTooltip)
			this.setControlTooltip(tooltip);
	}

	/**
	 * Do not call functions calling updateVisibility onMapChange but onGameAttributesChange,
	 * so that changes take effect when increasing the playercount as well.
	 */
	setEnabled(enabled)
	{
		this.enabled = enabled;
		this.updateVisibility();
	}

	setHidden(hidden)
	{
		this.hidden = hidden;
		this.updateVisibility();
	}

	updateVisibility()
	{
		let hidden =
			this.hidden ||
			this.playerIndex === undefined &&
				this.category != g_TabCategorySelected ||
			this.playerIndex !== undefined &&
				g_GameAttributes.settings && this.playerIndex >= g_GameAttributes.settings.PlayerData.length;

		if (this.frame)
			this.frame.hidden = hidden;

		if (hidden)
			return;

		let enabled = g_IsController && this.enabled;

		if (this.setControlHidden)
			this.setControlHidden(!enabled);

		if (this.label)
			this.label.hidden = !!enabled;
	}

	/**
	 * Returns whether the control specifies an order but didn't implement the function.
	 */
	addAutocompleteEntries(name, autocomplete)
	{
		if (this.autocompleteTitle)
			autocomplete[0].push(this.autocompleteTitle);

		if (!Number.isInteger(this.AutocompleteOrder))
			return;

		if (!this.getAutocompleteEntries)
		{
			error(name + " specifies AutocompleteOrder but didn't implement getAutocompleteEntries");
			return;
		}

		let newEntries = this.getAutocompleteEntries();
		if (newEntries)
			autocomplete[this.AutocompleteOrder] =
				(autocomplete[this.AutocompleteOrder] || []).concat(newEntries);
	}
}

GameSettingControl.prototype.TitleCaptionFormat =
	translateWithContext("Title for specific setting", "%(setting)s:");

/**
 * Derived classes can set this to a number to enable chat autocompleting of setting values.
 * Higher numbers are autocompleted first.
 */
GameSettingControl.prototype.AutocompleteOrder = undefined;
