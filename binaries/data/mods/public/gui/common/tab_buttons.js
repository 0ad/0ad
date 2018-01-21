/**
 * Number of categories.
 */
var g_TabCategoryCount;

/**
 * Index of the currently visible tab, set first tab as default.
 */
var g_TabCategorySelected = 0;

/**
 * Function to be executed when selecting a tab. The new category index is passed.
 */
var g_OnSelectTab;

/**
 * Create tab buttons.
 *
 * @param {Array} categoriesData - Arrays of objects containing for every tab a (translated) label and tooltip.
 * @param {number} buttonHeight - Vertical distance between the top and bottom of a button.
 * @param {number} spacing - Vertical distance between two buttons.
 * @param {function} onPress - Function to be executed when a button is pressed, it gets the new category index passed.
 * @param {function} onSelect - Function to be executed whenever the selection changes (so also for scrolling), it gets the new category index passed.
 */
function placeTabButtons(categoriesData, buttonHeight, spacing, onPress, onSelect)
{
	g_OnSelectTab = onSelect;
	g_TabCategoryCount = categoriesData.length;

	for (let category in categoriesData)
	{
		let button = Engine.GetGUIObjectByName("tabButton[" + category + "]");
		if (!button)
		{
			warn("Too few tab-buttons!");
			break;
		}

		button.hidden = false;

		let size = button.size;
		size.top = category * (buttonHeight + spacing) + spacing / 2;
		size.bottom = size.top + buttonHeight;
		button.size = size;
		button.tooltip = categoriesData[category].tooltip || "";
		button.onPress = (category => function() { onPress(category); })(+category);

		Engine.GetGUIObjectByName("tabButtonText[" + category + "]").caption = categoriesData[category].label;
	}

	selectPanel(g_TabCategorySelected);
}

/**
 * Show next/previous panel.
 * @param direction - +1/-1 for forward/backward.
 */
function selectNextTab(direction)
{
	selectPanel(g_TabCategorySelected === undefined ?
		direction > 0 ?
			0 :
			g_TabCategoryCount - 1 :
		(g_TabCategorySelected + direction + g_TabCategoryCount) % g_TabCategoryCount);
}

function selectPanel(category)
{
	g_TabCategorySelected = category;
	Engine.GetGUIObjectByName("tabButtons").children.forEach((button, j) => {
		button.sprite = category == j ? "ModernTabVerticalForeground" : "ModernTabVerticalBackground";
	});

	g_OnSelectTab(category);
}
