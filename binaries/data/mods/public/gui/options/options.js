var g_HasCallback = false;
var g_Controls;

var g_DependentLabelIndentation = 25;

function init(data)
{
	if (data && data.callback)
		g_HasCallback = true;
	g_Controls = {};

	let options = Engine.ReadJSONFile("gui/options/options.json");
	for (let category in options)
	{
		let lastSize;
		for (let i = 0; i < options[category].length; ++i)
		{
			let option = options[category][i];
			if (!option.label || !option.parameters || !option.parameters.config)
				continue;
			let body = Engine.GetGUIObjectByName(category + "[" + i + "]");
			let label = Engine.GetGUIObjectByName(category + "Label[" + i + "]");
			let config = option.parameters.config;
			g_Controls[config] = {
				"control": setupControl(option, i, category),
				"label": label,
				"type": option.type,
				"dependencies": option.dependencies || undefined,
				"parameters": option.parameters
			};
			label.caption = translate(option.label);
			label.tooltip = option.tooltip ? translate(option.tooltip) : "";
			// Move each element to the correct place.
			if (lastSize)
			{
				let newSize = new GUISize();
				newSize.left = lastSize.left;
				newSize.rright = lastSize.rright;
				newSize.top = lastSize.bottom;
				newSize.bottom = newSize.top + 26;
				body.size = newSize;
				lastSize = newSize;
			}
			else
				lastSize = body.size;

			let labelSize = label.size;
			labelSize.left = option.dependency ? g_DependentLabelIndentation : 0;
			labelSize.rright = g_Controls[config].control.size.rleft;
			label.size = labelSize;

			body.hidden = false;
		}
	}

	updateOptionPanel();
}

/**
 * Setup the apropriate control for a given option.
 *
 * @param option Structure containing the data to setup an option.
 * @param prefix Prefix to use when accessing control, for example "generalSetting" when the tickbox name is generalSettingTickbox[i].
 */
function setupControl(option, i, category)
{
	let control;
	let onUpdate;
	let key = option.parameters.config;

	switch (option.type)
	{
	case "boolean":
		control = Engine.GetGUIObjectByName(category + "Tickbox[" + i + "]");
		let checked;
		for (let param in option.parameters)
		{
			switch (param)
			{
			case "config":
				checked = Engine.ConfigDB_GetValue("user", key) == "true";
				break;
			case "function":
				break;
			default:
				warn("Unknown option source type '" + param + "'");
			}
		}

		onUpdate = function(key)
		{
			return function()
			{
				if (option.parameters.function)
					Engine[option.parameters.function](this.checked);
				Engine.ConfigDB_CreateValue("user", key, String(this.checked));
				Engine.ConfigDB_SetChanges("user", true);
				updateOptionPanel();
			};
		}(key);

		// Load final data to the control element.
		control.checked = checked;
		control.onPress = onUpdate;
		break;
	case "slider":
		control = Engine.GetGUIObjectByName(category + "Slider[" + i + "]");
		let value;
		let callbackFunction;
		let minvalue;
		let maxvalue;

		for (let param in option.parameters)
		{
			switch (param)
			{
			case "config":
				value = +Engine.ConfigDB_GetValue("user", key);
				break;
			case "function":
				if (Engine[option.parameters.function])
					callbackFunction = option.parameters.function;
				break;
			case "min":
				minvalue = +option.parameters.min;
				break;
			case "max":
				maxvalue = +option.parameters.max;
				break;
			default:
				warn("Unknown option source type '" + param + "'");
			}
		}

		onUpdate = function(key, callbackFunction, minvalue, maxvalue)
		{
			return function()
			{
				this.tooltip =
					(option.tooltip ? translate(option.tooltip) + "\n" : "") +
					sprintf(translateWithContext("slider number", "Value: %(val)s (min: %(min)s, max: %(max)s)"), {
						"val": +this.value.toFixed(2),
						"min": +minvalue.toFixed(2),
						"max": +maxvalue.toFixed(2)
					});

				if (+Engine.ConfigDB_GetValue("user", key) === this.value)
					return;
				Engine.ConfigDB_CreateValue("user", key, this.value);
				Engine.ConfigDB_SetChanges("user", true);
				if (callbackFunction)
					Engine[callbackFunction](+this.value);
				updateOptionPanel();
			};
		}(key, callbackFunction, minvalue, maxvalue);

		control.value = value;
		control.max_value = maxvalue;
		control.min_value = minvalue;
		control.onValueChange = onUpdate;
		break;
	case "number":
	case "string":
		control = Engine.GetGUIObjectByName(category + "Input[" + i + "]");
		let caption;
		let functionBody;
		let minval;
		let maxval;

		for (let param in option.parameters)
		{
			switch (param)
			{
			case "config":
				caption = Engine.ConfigDB_GetValue("user", key);
				break;
			case "function":
				if (Engine[option.parameters.function])
					functionBody = option.parameters.function;
				break;
			case "min":
				minval = option.parameters.min;
				break;
			case "max":
				maxval = option.parameters.max;
				break;
			default:
				warn("Unknown option source type '" + param + "'");
			}
		}

		onUpdate = function(key, functionBody, minval, maxval)
		{
			return function()
			{
				if (minval && +minval > +this.caption)
					this.caption = minval;
				if (maxval && +maxval < +this.caption)
					this.caption = maxval;
				if (Engine.ConfigDB_GetValue("user", key) == this.caption)
					return;
				Engine.ConfigDB_CreateValue("user", key, this.caption);
				Engine.ConfigDB_SetChanges("user", true);
				if (functionBody)
					Engine[functionBody](+this.caption);
				updateOptionPanel();
			};
		}(key, functionBody, minval, maxval);

		control.caption = caption;
		control.onTextEdit = onUpdate;
		break;
	case "dropdown":
	{
		control = Engine.GetGUIObjectByName(category + "Dropdown[" + i + "]");
		control.onSelectionChange = function(){};  // just the time to setup the value
		let config;
		let callbackFunction;

		for (let param in option.parameters)
		{
			switch (param)
			{
			case "config":
				config = Engine.ConfigDB_GetValue("user", key);
				break;
			case "function":
				if (Engine[option.parameters.function])
					callbackFunction = option.parameters.function;
				break;
			case "list":
				control.list = option.parameters.list.map(e => translate(e.label));
				let values = option.parameters.list.map(e => e.value);
				control.list_data = values;
				control.selected = values.map(String).indexOf(config);
				break;
			default:
				warn("Unknown option source type '" + param + "'");
			}
		}

		onUpdate = function(key, callbackFunction)
		{
			return function()
			{
				Engine.ConfigDB_CreateValue("user", key, this.list_data[this.selected]);
				Engine.ConfigDB_SetChanges("user", true);
				if (callbackFunction)
					Engine[callbackFunction](this.list_data[this.selected]);
				updateOptionPanel();
			};
		}(key, callbackFunction);

		control.onSelectionChange = onUpdate;
		break;
	}
	default:
		warn("Unknown option type " + option.type + ", assuming string.");
		control = Engine.GetGUIObjectByName(category + "Input[" + i + "]");
		break;
	}
	control.hidden = false;

	if (option.type == "slider")
		control.onValueChange();
	else
		control.tooltip = option.tooltip ? translate(option.tooltip) : "";

	return control;
}

function updateOptionPanel()
{
	for (let item in g_Controls)
	{
		let enabled =
			!g_Controls[item].dependencies ||
			g_Controls[item].dependencies.every(config => Engine.ConfigDB_GetValue("user", config) == "true");

		g_Controls[item].control.enabled = enabled;
		g_Controls[item].label.enabled = enabled;
	}

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
	for (let item in g_Controls)
		Engine.ConfigDB_RemoveValue("user", item);

	Engine.ConfigDB_WriteFile("user", "config/user.cfg");
	revertChanges();
}

function revertChanges()
{
	Engine.ConfigDB_Reload("user");
	for (let item in g_Controls)
	{
		let control = g_Controls[item];
		if (control.parameters.function)
		{
			let value = Engine.ConfigDB_GetValue("user", item);
			Engine[control.parameters.function](
				(control.type == "string" || control.type == "dropdown") ?
					value :
				control.type == "boolean" ?
					value == "true" :
					+value);
		}
	}
	Engine.ConfigDB_SetChanges("user", false);
	init();
}

function saveChanges()
{
	Engine.ConfigDB_WriteFile("user", "config/user.cfg");
	Engine.ConfigDB_SetChanges("user", false);
	updateOptionPanel();
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
