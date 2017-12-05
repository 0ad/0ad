/**
 * Since the AI context can't access JSON functions, it gets passed an object
 * containing the information from `GuiInterface.js::GetSimulationState()`.
 */
function Resources()
{
	this.resourceData = [];
	this.resourceDataObj = {};
	this.resourceCodes = [];
	this.resourceNames = {};

	for (let filename of Engine.ListDirectoryFiles("simulation/data/resources/", "*.json", false))
	{
		let data = Engine.ReadJSONFile(filename);
		if (!data)
			continue;

		if (data.code != data.code.toLowerCase())
			warn("Resource codes should use lower case: " + data.code);

		// Treasures are supported for every specified resource
		if (data.code == "treasure")
		{
			error("Encountered resource with reserved keyword: " + data.code);
			continue;
		}

		this.resourceData.push(data);
		this.resourceDataObj[data.code] = data;
		this.resourceCodes.push(data.code);
		this.resourceNames[data.code] = data.name;
		for (let subres in data.subtypes)
			this.resourceNames[subres] = data.subtypes[subres];
	}

	// Sort arrays by specified order
	let resSort = (a, b) =>
		a.order < b.order ? -1 :
		a.order > b.order ? +1 : 0;

	this.resourceData.sort(resSort);
	this.resourceCodes.sort((a, b) => resSort(
		this.resourceData.find(resource => resource.code == a),
		this.resourceData.find(resource => resource.code == b)
	));

	deepfreeze(this.resourceData);
	deepfreeze(this.resourceDataObj);
	deepfreeze(this.resourceCodes);
	deepfreeze(this.resourceNames);
}

/**
 * Returns the objects defined in the JSON files for all availbale resources,
 * ordered as defined in these files.
 */
Resources.prototype.GetResources = function()
{
	return this.resourceData;
};

/**
 * Returns the object defined in the JSON file for the given resource.
 */
Resources.prototype.GetResource = function(type)
{
	return this.resourceDataObj[type];
};

/**
 * Returns an array containing all resource codes ordered as defined in the resource files.
 * For example ["food", "wood", "stone", "metal"].
 */
Resources.prototype.GetCodes = function()
{
	return this.resourceCodes;
};

/**
 * Returns an object mapping resource codes to translatable resource names. Includes subtypes.
 * For example { "food": "Food", "fish": "Fish", "fruit": "Fruit", "metal": "Metal", ... }
 */
Resources.prototype.GetNames = function()
{
	return this.resourceNames;
};
