/**
 * Builds a RelaxRNG schema based on currently valid elements.
 *
 * To prevent validation errors, disabled resources are included in the schema.
 *
 * @param datatype - The datatype of the element
 * @param additional - Array of additional data elements. Time, xp, treasure, etc.
 * @param subtypes - If true, resource subtypes will be included as well.
 * @return RelaxNG schema string
 */
Resources.prototype.BuildSchema = function(datatype, additional = [], subtypes = false)
{
	if (!datatype)
		return "";

	switch (datatype)
	{
	case "decimal":
	case "nonNegativeDecimal":
	case "positiveDecimal":
		datatype = "<ref name='" + datatype + "'/>";
		break;

	default:
		datatype = "<data type='" + datatype + "'/>";
	}

	let resCodes = this.resourceData.map(resource => resource.code);
	let schema = "";
	for (let res of resCodes.concat(additional))
		schema +=
			"<optional>" +
				"<element name='" + res + "'>" +
					datatype +
				"</element>" +
			"</optional>";

	if (!subtypes)
		return "<interleave>" + schema + "</interleave>";

	for (let res of this.resourceData)
		for (let subtype in res.subtypes)
			schema +=
				"<optional>" +
					"<element name='" + res.code + "." + subtype + "'>" +
						datatype +
					"</element>" +
				"</optional>";

	if (additional.indexOf("treasure") !== -1)
		for (let res of resCodes)
			schema +=
				"<optional>" +
					"<element name='" + "treasure." + res + "'>" +
						datatype +
					"</element>" +
				"</optional>";

	return "<interleave>" + schema + "</interleave>";
};

/**
 * Builds the value choices for a RelaxNG `<choice></choice>` object, based on currently valid resources.
 *
 * @oaram subtypes - If set to true, the choices returned will be resource subtypes, rather than main types
 * @param treasure - If set to true, the pseudo resource 'treasure' (or its subtypes) will be included
 * @return String of RelaxNG Schema `<choice/>` values.
 */
Resources.prototype.BuildChoicesSchema = function(subtypes = false, treasure = false)
{
	let schema = "";

	if (!subtypes)
	{
		let resCodes = this.resourceData.map(resource => resource.code);
		if (treasure)
			resCodes.push("treasure");
		for (let res of resCodes)
			schema += "<value>" + res + "</value>";
	}
	else
		for (let res of this.resourceData)
		{
			for (let subtype in res.subtypes)
				schema += "<value>" + res.code + "." + subtype + "</value>";
			if (treasure)
				schema += "<value>" + "treasure." + res.code + "</value>";
		}

	return "<choice>" + schema + "</choice>";
};

Resources = new Resources();
