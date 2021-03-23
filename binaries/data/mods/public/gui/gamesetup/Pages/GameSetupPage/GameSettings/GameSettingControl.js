/**
 * The GameSettingControl is an abstract class that is inherited by game-setting control classes specific to a GUI-object type,
 * such as the GameSettingControlCheckbox or GameSettingControlDropdown.
 *
 * The purpose of these classes is to control one logical game setting.
 * The base classes allow implementing that while avoiding duplication.
 *
 * GameSettingControl classes watch for g_GameSettings property changes,
 * and re-render accordingly. They also trigger changes in g_GameSettings.
 *
 * The GameSettingControl classes are responsible for triggering network synchronisation,
 * and for updating the whole gamesetup layout when necessary.
 */
class GameSettingControl /* extends Profilable /* Uncomment to profile controls without hassle. */
{
	constructor(gameSettingControlManager, category, playerIndex, setupWindow)
	{
		// Store arguments
		{
			this.category = category;
			this.playerIndex = playerIndex;

			this.setupWindow = setupWindow;
			this.gameSettingsController = setupWindow.controls.gameSettingsController;
			this.mapCache = setupWindow.controls.mapCache;
			this.mapFilters = setupWindow.controls.mapFilters;
			this.netMessages = setupWindow.controls.netMessages;
			this.playerAssignmentsController = setupWindow.controls.playerAssignmentsController;
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

		if (this.onLoad)
			this.setupWindow.registerLoadHandler(this.onLoad.bind(this));

		if (this.onPlayerAssignmentsChange)
			this.playerAssignmentsController.registerPlayerAssignmentsChangeHandler(this.onPlayerAssignmentsChange.bind(this));
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

	setEnabled(enabled)
	{
		this.enabled = enabled;
		this.updateVisibility();
	}

	setHidden(hidden)
	{
		this.hidden = hidden;
		// Trigger a layout update to reposition items.
		this.gameSettingsController.updateLayout();
	}

	updateVisibility()
	{
		let hidden =
			this.hidden ||
			this.playerIndex === undefined &&
				this.category != g_TabCategorySelected ||
					this.playerIndex !== undefined &&
					this.playerIndex >= g_GameSettings.playerCount.nbPlayers;

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
