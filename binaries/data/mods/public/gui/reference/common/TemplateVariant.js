/**
 * Enum-type class that defines various template variant types.
 */
class TemplateVariant
{
	/**
	 * @param passthru Signifies if we should pass though to the base template when generating build lists.
	 */
	constructor(name, passthru=true)
	{
		this.name = name;
		this.passthru = passthru;

		TemplateVariant[name] = this;
	}

	static registerType(name, passthru=true)
	{
		TemplateVariant[name] = new TemplateVariant(name, passthru);
	}

	toString()
	{
		return this.constructor.name + "." + this.name;
	}
}

/**
 * Registered Template Variants.
 * New variants add themselves as static properties to the main class.
 */
TemplateVariant.registerType("base");
TemplateVariant.registerType("unknown");
TemplateVariant.registerType("upgrade", false);
TemplateVariant.registerType("promotion", false);
TemplateVariant.registerType("unlockedByTechnology");
TemplateVariant.registerType("trainable");
