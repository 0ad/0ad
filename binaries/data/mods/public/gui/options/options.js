/**
 * Translated JSON file contents.
 */
var g_Options;

/**
 * Numerical index of the chosen category.
 */
var g_SelectedCategory;

/**
 * Remember whether to unpause running singleplayer games.
 */
var g_HasCallback;

/**
 * Vertical size of a tab button.
 */
var g_TabButtonHeight = 30;

/**
 * Vertical space between two tab buttons.
 */
var g_TabButtonDist = 5;

/**
 * Vertical distance between the top of the page and the first option.
 */
var g_OptionControlOffset = 5;

/**
 * Vertical size of each option control.
 */
var g_OptionControlHeight = 26;

/**
 * Vertical distance between two consecutive options.
 */
var g_OptionControlDist = 2;

/**
 * Horizontal indentation to distinguish options that depend on another option.
 */
var g_DependentLabelIndentation = 25;

/**
 * Defines the parsing of config strings and GUI control interaction for the different option types.
 *
 * @property configToValue - parses a string from the user config to a value of the declared type.
 * @property valueToGui - sets the GUI control to display the given value.
 * @property guiToValue - returns the value of the GUI control.
 * @property guiSetter - event name that should be considered a value change of the GUI control.
 * @property initGUI - sets properties of the GUI control that are independent of the current value.
 * @property sanitizeValue - ensures that a given value is in the defined value range.
 * @property tooltip - appends a custom tooltip to the given option description depending on the current value.
 */
var g_OptionType = {
	"boolean":
	{
		"configToValue": config => config == "true",
		"valueToGui": (value, control) => {
			control.checked = value;
		},
		"guiToValue": control => control.checked,
		"guiSetter": "onPress"
	},
	"string":
	{
		"configToValue": value => value,
		"valueToGui": (value, control) => {
			control.caption = value;
		},
		"guiToValue": control => control.caption,
		"guiSetter": "onTextEdit"
	},
	"number":
	{
		"configToValue": value => +value,
		"valueToGui": (value, control) => {
			control.caption = value;
		},
		"guiToValue": control => +control.caption,
		"guiSetter": "onTextEdit",
		"sanitizeValue": (value, option) =>
			Math.min(option.max || Infinity,
			Math.max(option.min || -Infinity, value)),
		"tooltip": (value, option) =>
			sprintf(
				option.min !== undefined && option.max !== undefined ?
					translateWithContext("option number", "Min: %(min)s, Max: %(max)s") :
				option.min !== undefined && option.max === undefined ?
					translateWithContext("option number", "Min: %(min)s") :
				option.min === undefined && option.max !== undefined ?
					translateWithContext("option number", "Max: %(max)s") :
					"",
				{
					"min": option.min,
					"max": option.max
				})
	},
	"dropdown":
	{
		"configToValue": value => value,
		"valueToGui": (value, control) => {
			control.selected = control.list_data.indexOf(value);
		},
		"guiToValue": control => control.list_data[control.selected],
		"guiSetter": "onSelectionChange",
		"initGUI": (option, control) => {
			control.list = option.list.map(e => e.label);
			control.list_data = option.list.map(e => e.value);
		},
	},
	"slider":
	{
		"configToValue": value => +value,
		"valueToGui": (value, control) => {
			control.value = +value;
		},
		"guiToValue": control => control.value,
		"guiSetter": "onValueChange",
		"initGUI": (option, control) => {
			control.max_value = option.max;
			control.min_value = option.min;
		},
		"tooltip": (value, option) =>
			sprintf(translateWithContext("slider number", "Value: %(val)s (min: %(min)s, max: %(max)s)"), {
				"val": value.toFixed(2),
				"min": option.min.toFixed(2),
				"max": option.max.toFixed(2)
			})
	}
};

function init(data, hotloadData)
{
	g_HasCallback = hotloadData && hotloadData.callback || data && data.callback;
	g_SelectedCategory = hotloadData ? hotloadData.selectedCategory : 0;

	g_Options = Engine.ReadJSONFile("gui/options/options.json");
	translateObjectKeys(g_Options, ["label", "tooltip"]);
	deepfreeze(g_Options);

	placeTabButtons();
	displayOptions();
}

function getHotloadData()
{
	return {
		"selectedCategory": g_SelectedCategory,
		"callback": g_HasCallback
	};
}

function placeTabButtons()
{
	for (let category in g_Options)
	{
		let button = Engine.GetGUIObjectByName("tabButton[" + category + "]");
		if (!button)
		{
			warn("Too few tab-buttons!");
			break;
		}

		button.hidden = false;

		let size = button.size;
		size.top = category * (g_TabButtonHeight + g_TabButtonDist);
		size.bottom = size.top + g_TabButtonHeight;
		button.size = size;
		button.tooltip = g_Options[category].tooltip || "";

		button.onPress = (category => function() {
			g_SelectedCategory = category;
			displayOptions();
		})(category);

		Engine.GetGUIObjectByName("tabButtonText[" + category + "]").caption = g_Options[category].label;
	}
}

/**
 * Sets up labels and controls of all options of the currently selected category.
 */
function displayOptions()
{
	// Highlight the selected tab
	Engine.GetGUIObjectByName("tabButtons").children.forEach((button, i) => {
		button.sprite = i == g_SelectedCategory ? "ModernTabVerticalForeground" : "ModernTabVerticalBackground";
	});

	// Hide all controls
	for (let body of Engine.GetGUIObjectByName("option_controls").children)
	{
		body.hidden = true;
		for (let control of body.children)
			control.hidden = true;
	}

	// Initialize label and control of each option for this category
	for (let i = 0; i < g_Options[g_SelectedCategory].options.length; ++i)
	{
		// Position vertically
		let body = Engine.GetGUIObjectByName("option_control[" + i + "]");
		let bodySize = body.size;
		bodySize.top = g_OptionControlOffset + i * (g_OptionControlHeight + g_OptionControlDist);
		bodySize.bottom = bodySize.top + g_OptionControlHeight;
		body.size = bodySize;
		body.hidden = false;

		// Load option data
		let option = g_Options[g_SelectedCategory].options[i];
		let optionType = g_OptionType[option.type];
		let value = optionType.configToValue(Engine.ConfigDB_GetValue("user", option.config));

		// Setup control
		let control = Engine.GetGUIObjectByName("option_control_" + option.type + "[" + i + "]");
		control.tooltip = option.tooltip + "\n" + (optionType.tooltip && optionType.tooltip(value, option));
		control.hidden = false;

		if (optionType.initGUI)
			optionType.initGUI(option, control);

		control[optionType.guiSetter] = function() {};
		optionType.valueToGui(value, control);

		control[optionType.guiSetter] = function() {

			let value = optionType.guiToValue(control);
			if (optionType.sanitizeValue)
			{
				value = optionType.sanitizeValue(value, option);
				optionType.valueToGui(value, control);
			}

			control.tooltip = option.tooltip + "\n" + (optionType.tooltip && optionType.tooltip(value, option));

			Engine.ConfigDB_CreateValue("user", option.config, String(value));
			Engine.ConfigDB_SetChanges("user", true);

			if (option.function)
				Engine[option.function](value);

			enableButtons();
		};

		// Setup label
		let label = Engine.GetGUIObjectByName("option_label[" + i + "]");
		label.caption = option.label;
		label.tooltip = option.tooltip;
		label.hidden = false;

		let labelSize = label.size;
		labelSize.left = option.dependencies ? g_DependentLabelIndentation : 0;
		labelSize.rright = control.size.rleft;
		label.size = labelSize;
	}

	enableButtons();
}

/**
 * Enable exactly the buttons whose dependencies are met.
 */
function enableButtons()
{
	g_Options[g_SelectedCategory].options.forEach((option, i) => {

		let enabled =
			!option.dependencies ||
			option.dependencies.every(config => Engine.ConfigDB_GetValue("user", config) == "true");

		Engine.GetGUIObjectByName("option_label[" + i + "]").enabled = enabled;
		Engine.GetGUIObjectByName("option_control_" + option.type + "[" + i + "]").enabled = enabled;
	});

	let hasChanges = Engine.ConfigDB_HasChanges("user");
	Engine.GetGUIObjectByName("revertChanges").enabled = hasChanges;
	Engine.GetGUIObjectByName("saveChanges").enabled = hasChanges;
}

function setDefaults()
{
	messageBox(
		500, 200,
		translate("Resetting the options will erase your saved settings. Do you want to continue?"),
		translate("Warning"),
		[translate("No"), translate("Yes")],
		[null, reallySetDefaults]
	);
}

function reallySetDefaults()
{
	for (let category in g_Options)
		for (let option of g_Options[category].options)
			Engine.ConfigDB_RemoveValue("user", option.config);

	Engine.ConfigDB_WriteFile("user", "config/user.cfg");
	revertChanges();
}

function revertChanges()
{
	Engine.ConfigDB_Reload("user");
	Engine.ConfigDB_SetChanges("user", false);

	for (let category in g_Options)
		for (let option of g_Options[category].options)
			if (option.function)
				Engine[option.function](
					g_OptionType[option.type].configToValue(
						Engine.ConfigDB_GetValue("user", option.config)));

	displayOptions();
}

function saveChanges()
{
	Engine.ConfigDB_WriteFile("user", "config/user.cfg");
	Engine.ConfigDB_SetChanges("user", false);
	enableButtons();
}

/**
 * Close GUI page and call callbacks if they exist.
 **/
function closePage()
{
	if (Engine.ConfigDB_HasChanges("user"))
	{
		messageBox(
			500, 200,
			translate("You have unsaved changes, do you want to close this window?"),
			translate("Warning"),
			[translate("No"), translate("Yes")],
			[null, closePageWithoutConfirmation]
		);
	}
	else
		closePageWithoutConfirmation();
}

function closePageWithoutConfirmation()
{
	if (g_HasCallback)
		Engine.PopGuiPageCB();
	else
		Engine.PopGuiPage();
}
