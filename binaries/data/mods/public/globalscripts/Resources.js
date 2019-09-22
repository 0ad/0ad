/**
 * This class provides a cache to all resource names and properties defined by the JSON files.
 */
function Resources()
{
	this.resourceData = [];
	this.resourceDataObj = {};
	this.resourceCodes = [];
	this.resourceNames = {};
	this.resourceCodesByProperty = {};

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

		for (let property in data.properties)
		{
			if (!this.resourceCodesByProperty[data.properties[property]])
				this.resourceCodesByProperty[data.properties[property]] = [];
			this.resourceCodesByProperty[data.properties[property]].push(data.code);
		}
	}

	// Sort arrays by specified order
	let resDataSort = (a, b) => a.order < b.order ? -1 : a.order > b.order ? +1 : 0;
	let resSort = (a, b) => resDataSort(
		this.resourceData.find(resource => resource.code == a),
		this.resourceData.find(resource => resource.code == b)
	);

	this.resourceData.sort(resDataSort);
	this.resourceCodes.sort(resSort);
	for (let property in this.resourceCodesByProperty)
		this.resourceCodesByProperty[property].sort(resSort);

	deepfreeze(this.resourceData);
	deepfreeze(this.resourceDataObj);
	deepfreeze(this.resourceCodes);
	deepfreeze(this.resourceNames);
	deepfreeze(this.resourceCodesByProperty);
}

/**
 * Returns the objects defined in the JSON files for all available resources,
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
 * @return {string[]} - Data of the form [ "food", "wood", ... ].
 */
Resources.prototype.GetCodes = function()
{
	return this.resourceCodes;
};

/**
 * Returns an array containing all barterable resource codes ordered as defined in the resource files.
 * @return {string[]} - Data of the form [ "food", "wood", ... ].
 */
Resources.prototype.GetBarterableCodes = function()
{
	return this.resourceCodesByProperty.barterable || [];
};

/**
 * Returns an array containing all tradable resource codes ordered as defined in the resource files.
 * @return {string[]} - Data of the form [ "food", "wood", ... ].
 */
Resources.prototype.GetTradableCodes = function()
{
	return this.resourceCodesByProperty.tradable || [];
};

/**
 * Returns an array containing all tributable resource codes ordered as defined in the resource files.
 * @return {string[]} - Data of the form [ "food", "wood", ... ].
 */
Resources.prototype.GetTributableCodes = function()
{
	return this.resourceCodesByProperty.tributable || [];
};

/**
 * Returns an object mapping resource codes to translatable resource names. Includes subtypes.
 * For example { "food": "Food", "fish": "Fish", "fruit": "Fruit", "metal": "Metal", ... }
 */
Resources.prototype.GetNames = function()
{
	return this.resourceNames;
};
