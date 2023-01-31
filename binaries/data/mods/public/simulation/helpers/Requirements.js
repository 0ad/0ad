function RequirementsHelper() {}

RequirementsHelper.prototype.DEFAULT_RECURSION_DEPTH = 1;

RequirementsHelper.prototype.EntityRequirementsSchema =
	"<element name='Entities' a:help='Entities that need to be controlled.'>" +
		"<oneOrMore>" +
			"<element a:help='Class of entity that needs to be controlled.'>" +
				"<anyName/>" +
				"<oneOrMore>" +
					"<choice>" +
						"<element name='Count' a:help='Number of entities required.'>" +
							"<data type='nonNegativeInteger'/>" +
						"</element>" +
						"<element name='Variants' a:help='Number of different entities of this class required.'>" +
							"<data type='nonNegativeInteger'/>" +
						"</element>" +
					"</choice>" +
				"</oneOrMore>" +
			"</element>" +
		"</oneOrMore>" +
	"</element>";

RequirementsHelper.prototype.TechnologyRequirementsSchema =
	"<element name='Techs' a:help='White-space separated list of technologies that need to be researched. ! negates a tech.'>" +
		"<attribute name='datatype'>" +
			"<value>tokens</value>" +
		"</attribute>" +
		"<text/>" +
	"</element>";

/**
 * @param {number} recursionDepth - How deep we recurse.
 * @return {string} - A RelaxRNG schema for requirements.
 */
RequirementsHelper.prototype.RequirementsSchema = function(recursionDepth)
{
	return "" +
		"<oneOrMore>" +
			this.ChoicesSchema(--recursionDepth) +
		"</oneOrMore>";
};

/**
 * @param {number} recursionDepth - How deep we recurse.
 * @return {string} - A RelaxRNG schema for chosing requirements.
 */
RequirementsHelper.prototype.ChoicesSchema = function(recursionDepth)
{
	const allAnySchema = recursionDepth > 0 ? "" +
		"<element name='All' a:help='Requires all of the conditions to be met.'>" +
			this.RequirementsSchema(recursionDepth) +
		"</element>" +
		"<element name='Any' a:help='Requires at least one of the following conditions met.'>" +
			this.RequirementsSchema(recursionDepth) +
		"</element>" : "";

	return "" +
		"<choice>" +
			allAnySchema +
			this.EntityRequirementsSchema +
			this.TechnologyRequirementsSchema +
		"</choice>";
};

/**
 * @param {number} recursionDepth - How deeply recursive we build the schema.
 * @return {string} - A RelaxRNG schema for requirements.
 */
RequirementsHelper.prototype.BuildSchema = function(recursionDepth = this.DEFAULT_RECURSION_DEPTH)
{
	return "" +
		"<element name='Requirements' a:help='The requirements that ought to be met before this entity can be produced.'>" +
			"<optional>" +
				this.ChoicesSchema(recursionDepth) +
			"</optional>" +
			"<optional>" +
				"<element name='Tooltip' a:help='A tooltip explaining the requirements.'>" +
					"<text/>" +
				"</element>" +
			"</optional>" +
		"</element>";
};

/**
 * @param {Object} template - The requirements template as defined above.
 * @param {number} playerID -
 * @return {boolean} -
 */
RequirementsHelper.prototype.AreRequirementsMet = function(template, playerID)
{
	if (!template || !Object.keys(template).length)
		return true;

	const cmpTechManager = QueryPlayerIDInterface(playerID, IID_TechnologyManager);
	return cmpTechManager && this.AllRequirementsMet(template, cmpTechManager);
};

/**
 * @param {Object} template - The requirements template for "all".
 * @param {component} cmpTechManager -
 * @return {boolean} -
 */
RequirementsHelper.prototype.AllRequirementsMet = function(template, cmpTechManager)
{
	for (const requirementType in template)
	{
		const requirement = template[requirementType];
		if (requirementType === "All" && !this.AllRequirementsMet(requirement, cmpTechManager))
			return false;
		if (requirementType === "Any" && !this.AnyRequirementsMet(requirement, cmpTechManager))
			return false;
		if (requirementType === "Entities")
		{
			for (const className in requirement)
			{
				const entReq = requirement[className];
				if ("Count" in entReq && (!(className in cmpTechManager.classCounts) || cmpTechManager.classCounts[className] < entReq.Count))
					return false;
				if ("Variants" in entReq && (!(className in cmpTechManager.typeCountsByClass) || Object.keys(cmpTechManager.typeCountsByClass[className]).length < entReq.Variants))
					return false;
			}
		}
		if (requirementType === "Techs" && requirement._string)
			for (const tech of requirement._string.split(" "))
				if (tech[0] === "!" ? cmpTechManager.IsTechnologyResearched(tech.substring(1)) :
					!cmpTechManager.IsTechnologyResearched(tech))
					return false;
	}
	return true;
};

/**
 * @param {Object} template - The requirements template for "any".
 * @param {component} cmpTechManager -
 * @return {boolean} -
 */
RequirementsHelper.prototype.AnyRequirementsMet = function(template, cmpTechManager)
{
	for (const requirementType in template)
	{
		const requirement = template[requirementType];
		if (requirementType === "All" && this.AllRequirementsMet(requirement, cmpTechManager))
			return true;
		if (requirementType === "Any" && this.AnyRequirementsMet(requirement, cmpTechManager))
			return true;
		if (requirementType === "Entities")
		{
			for (const className in requirement)
			{
				const entReq = requirement[className];
				if ("Count" in entReq && className in cmpTechManager.classCounts && cmpTechManager.classCounts[className] >= entReq.Count)
					return true;
				if ("Variants" in entReq && className in cmpTechManager.typeCountsByClass && Object.keys(cmpTechManager.typeCountsByClass[className]).length >= entReq.Variants)
					return true;
			}
		}
		if (requirementType === "Techs" && requirement._string)
			for (const tech of requirement._string.split(" "))
				if (tech[0] === "!" ? !cmpTechManager.IsTechnologyResearched(tech.substring(1)) :
					cmpTechManager.IsTechnologyResearched(tech))
					return true;
	}
	return false;
};

Engine.RegisterGlobal("RequirementsHelper", new RequirementsHelper());
