var g_hasCallback = false;
var g_controls;

function init(data)
{
	if (data && data.callback)
		g_hasCallback = true;
	let revert = data && data.revert;
	g_controls = [];

	var options = Engine.ReadJSONFile("gui/options/options.json");
	for (let category of Object.keys(options))
	{
		let lastSize;
		for (let i = 0; i < options[category].length; ++i)
		{
			let option = options[category][i];
			if (!option.label)
				continue;
			let body = Engine.GetGUIObjectByName(category + "[" + i + "]");
			let label = Engine.GetGUIObjectByName(category + "Label[" + i + "]");
			setupControl(option, i, category, revert);
			label.caption = translate(option.label);
			label.tooltip = option.tooltip ? translate(option.tooltip) : "";
			// Move each element to the correct place.
			if (i > 0)
			{
				let newSize = new GUISize();
				newSize.left = lastSize.left;
				newSize.rright = lastSize.rright;
				newSize.top = lastSize.bottom;
				newSize.bottom = newSize.top + 25;
				body.size = newSize;
				lastSize = newSize;
			}
			else
				lastSize = body.size;
			// Show element.
			body.hidden = false;
		}
	}

	if (!revert)
		updateStatus();
}

/**
 * Setup the apropriate control for a given option.
 *
 * @param option Structure containing the data to setup an option.
 * @param prefix Prefix to use when accessing control, for example "generalSetting" when the tickbox name is generalSettingTickbox[i].
 */
function setupControl(option, i, category, revert)
{
	var control;
	var onUpdate;

	switch (option.type)
	{
	case "boolean":
		// More space for the label
		let text = Engine.GetGUIObjectByName(category + "Label[" + i + "]");
		let size = text.size;
		size.rright = 87;
		text.size = size;
		control = Engine.GetGUIObjectByName(category + "Tickbox[" + i + "]");
		var checked;
		var keyRenderer;
		var keyConfig;
		var functionBody;

		for (let param of Object.keys(option.parameters))
		{
			switch (param)
			{
			case "config":
				keyConfig = option.parameters.config;
				if (checked === undefined || revert)
					checked = Engine.ConfigDB_GetValue("user", keyConfig) === "true";
				else if ((Engine.ConfigDB_GetValue("user", keyConfig) === "true") !== checked)
					Engine.ConfigDB_CreateValue("user", keyConfig, String(checked));
				break;
			case "renderer":
				keyRenderer = option.parameters.renderer;
				if (!Engine["Renderer_Get" + keyRenderer + "Enabled"])
				{
					warn(" invalid renderer key " + keyRenderer);
					keyRenderer = undefined;
					break;
				}
				if (checked === undefined)
					checked = Engine["Renderer_Get" + keyRenderer + "Enabled"]();
				else if (Engine["Renderer_Get" + keyRenderer + "Enabled"]() !== checked)
					Engine["Renderer_Set" + keyRenderer + "Enabled"](checked);
				break;
			default:
				warn("Unknown option source type '" + param + "'");
			}
		}

		onUpdate = function(keyRenderer, keyConfig)
		{
			return function()
			{
				if (keyRenderer)
					Engine["Renderer_Set" + keyRenderer + "Enabled"](this.checked);
				if (keyConfig)
					Engine.ConfigDB_CreateValue("user", keyConfig, String(this.checked));
				updateStatus(true);
			};
		}(keyRenderer, keyConfig);

		// Load final data to the control element.
		control.checked = checked;
		control.onPress = onUpdate;
		break;
	case "number":
		// TODO: Slider
	case "string":
		control = Engine.GetGUIObjectByName(category + "Input[" + i + "]");
		var caption;
		var key;
		var functionBody;
		var minval;
		var maxval;

		for (let param of Object.keys(option.parameters))
		{
			switch (param)
			{
			case "config":
				key = option.parameters.config;
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
				warn("Unknown option source type '" + action + "'");
			}
		}

		// as the enter key is not necessarily pressed after modifying an entry, we will register the input also
		// - when the mouse leave the control (MouseLeave event)
		// - or when saving or closing the window (registerChanges function)
		// so we must ensure that something has indeed been modified
		onUpdate = function(key, functionBody, minval, maxval)
		{
			return function()
			{
				if (minval && +minval > +this.caption)
					this.caption = minval;
				if (maxval && +maxval < +this.caption)
					this.caption = maxval;
				if (Engine.ConfigDB_GetValue("user", key) === this.caption)
					return;
				Engine.ConfigDB_CreateValue("user", key, this.caption);
				if (functionBody)
					Engine[functionBody](+this.caption);
				updateStatus(true);
			};
		}(key, functionBody, minval, maxval);

		control.caption = caption;
		control.onPress = onUpdate;
		control.onMouseLeave = onUpdate;
		g_controls.push(control);
		break;
	case "dropdown":
		control = Engine.GetGUIObjectByName(category + "Dropdown[" + i + "]");
		var caption;
		var key;
		var functionBody;
		var minval;
		var maxval;

		for (let param of Object.keys(option.parameters))
		{
			switch (param)
			{
			case "config":
				key = option.parameters.config;
				let val = +Engine.ConfigDB_GetValue("user", key);
				if (key === "materialmgr.quality")
					val = val > 5 ? 2 : val > 2 ? 1 : 0;
				control.selected = val;
				break;
			case "list":
				control.list = option.parameters.list.map(e => translate(e));
				break;
			case "list_data":
				control.list_data = option.parameters.list_data;
				break;
			default:
				warn("Unknown option source type '" + action + "'");
			}
		}

		onUpdate = function(key)
		{
			return function()
			{
				let val = this.selected;
				if (key === "materialmgr.quality")
					val = val == 0 ? 2 : val == 1 ? 5 : 8;
				Engine.ConfigDB_CreateValue("user", key, val);
				updateStatus(true);
			};
		}(key);

		control.onSelectionChange = onUpdate;
		break;
	default:
		warn("Unknown option type '" + option.type + "', assuming string. Valid types are 'number', 'string', or 'bool'.");
		control = Engine.GetGUIObjectByName(category + "Input[" + i + "]");
		break;
	}
	control.hidden = false;
	control.tooltip = option.tooltip ? translate(option.tooltip) : "";
	return control;
}

function updateStatus(val)
{
	if (typeof val == "boolean")
		Engine.ConfigDB_CreateValue("user", "nosave.haschanges", String(val));
	else
		val = Engine.ConfigDB_GetValue("user", "nosave.haschanges") === "true";

	Engine.GetGUIObjectByName("loadOptions").enabled = val;
	Engine.GetGUIObjectByName("saveOptions").enabled = val;
}

/**
 * Register changes of input (text and number) controls
 */
function registerChanges()
{
	for (let control of g_controls)
		control.onPress();
}

function revertChanges()
{
	Engine.ConfigDB_Reload("user");
	updateStatus(false);
	init({ "revert": true });
}

function saveChanges()
{
	registerChanges();
	Engine.ConfigDB_WriteFile("user", "config/user.cfg");
	updateStatus(false);
}

/**
 * Close GUI page and call callbacks if they exist.
 **/
function closePage()
{
	registerChanges();
	if (Engine.ConfigDB_GetValue("user", "nosave.haschanges") === "true")
	{
		let btCaptions = [translate("No"), translate("Yes")];
		let btCode = [null, function(){ closePageWithoutConfirmation(); }];
		messageBox(500, 200, translate("You have unsaved changes, do you want to close this window ?"),
			translate("Warning"), 0, btCaptions, btCode);
	}
	else
		closePageWithoutConfirmation();
}

function closePageWithoutConfirmation()
{
	if (g_hasCallback)
		Engine.PopGuiPageCB();
	else
		Engine.PopGuiPage();
}
