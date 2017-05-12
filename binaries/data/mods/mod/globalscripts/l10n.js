/**
 * These functions rely on the JS cache where possible and
 * should be prefered over the Engine.Translate ones to optimize the performance.
 */
var g_Translations = {};
var g_PluralTranslations = {};
var g_TranslationsWithContext = {};
var g_PluralTranslationsWithContext = {};

function isTranslatableString(message)
{
	return typeof message == "string" && !!message.trim();
}

/**
 * Translates the specified English message into the current language.
 */
function translate(message)
{
	if (!g_Translations[message])
		g_Translations[message] = Engine.Translate(message);

	return g_Translations[message];
}

/**
 * Translates the specified English message into the current language for the specified number.
 */
function translatePlural(singularMessage, pluralMessage, number)
{
	if (!g_PluralTranslations[singularMessage])
		g_PluralTranslations[singularMessage] = {};

	if (!g_PluralTranslations[singularMessage][number])
		g_PluralTranslations[singularMessage][number] = Engine.TranslatePlural(singularMessage, pluralMessage, number);

	return g_PluralTranslations[singularMessage][number];
}


/**
 * Translates the specified English message into the current language for the specified context.
 */
function translateWithContext(context, message)
{
	if (!g_TranslationsWithContext[context])
		g_TranslationsWithContext[context] = {}

	if (!g_TranslationsWithContext[context][message])
		g_TranslationsWithContext[context][message] = Engine.TranslateWithContext(context, message);

	return g_TranslationsWithContext[context][message];
}


/**
 * Translates the specified English message into the current language for the specified context and number.
 */
function translatePluralWithContext(context, singularMessage, pluralMessage, number)
{
	if (!g_PluralTranslationsWithContext[context])
		g_PluralTranslationsWithContext[context] = {};

	if (!g_PluralTranslationsWithContext[context][singularMessage])
		g_PluralTranslationsWithContext[context][singularMessage] = {};

	if (!g_PluralTranslationsWithContext[context][singularMessage][number])
		g_PluralTranslationsWithContext[context][singularMessage][number] =
			Engine.TranslatePluralWithContext(context, singularMessage, pluralMessage, number);

	return g_PluralTranslationsWithContext[context][singularMessage][number];
}

/**
 * The input object should contain either of the following properties:
 *
 *     • A ‘message’ property that contains a message to translate.
 *
 *     • A ‘list’ property that contains a list of messages to translate as a
 *     comma-separated list of translated.
 *
 * Optionally, the input object may contain a ‘context’ property. In that case,
 * the value of this property is used as translation context, that is, passed to
 * the translateWithContext(context, message) function.
 */
function translateMessageObject(object)
{
	let trans = translate;
	if (object.context)
		trans = msg => translateWithContext(object.context, msg);

	if (object.message)
		object = trans(object.message);
	else if (object.list)
		object = object.list.map(trans).join(translateWithContext("enumeration", ", "));

	return object;
}

/**
 * Translates any string value in the specified JavaScript object
 * that is associated with a key included in the specified keys array.
 *
 * it accepts an object in the form of
 *
 * {
 *   translatedString1: "my first message",
 *   unTranslatedString1: "some English string",
 *   ignoredObject: {
 *     translatedString2: "my second message",
 *     unTranslatedString2: "some English string"
 *   },
 *   translatedObject1: {
 *     message: "my third singular message",
 *     context: "message context",
 *   },
 *   translatedObject2: {
 *     list: ["list", "of", "strings"],
 *     context: "message context",
 *   },
 * }
 *
 * Together with a keys list to translate the strings and objects
 * ["translatedString1", "translatedString2", "translatedObject1",
 * "translatedObject2"]
 *
 * The result will be (f.e. in Dutch)
 * {
 *  translatedString1: "mijn eerste bericht",
 *   unTranslatedString1: "some English string",
 *   ignoredObject: {
 *     translatedString2: "mijn tweede bericht",
 *     unTranslatedString2: "some English string"
 *   },
 *   translatedObject1: "mijn derde bericht",
 *   translatedObject2: "lijst, van, teksten",
 * }
 *
 * So you see that the keys array can also contain lower-level keys,
 * And that you can include objects in the keys array to translate
 * them with a context, or to join a list of translations.
 *
 * Also, the keys array may be an object where properties are keys to translate
 * and values are translation contexts to use for each key.
 */
function translateObjectKeys(object, keys)
{
	if (keys instanceof Array)
	{
		for (let property in object)
		{
			if (keys.indexOf(property) > -1)
			{
				if (isTranslatableString(object[property]))
					object[property] = translate(object[property]);
				else if (object[property] instanceof Object)
					object[property] = translateMessageObject(object[property]);
			}
			else if (object[property] instanceof Object)
				translateObjectKeys(object[property], keys);
		}
	}
	// If ‘keys’ is not an array, it is an object where keys are properties to
	// translate and values are translation contexts to use for each key.
	// An empty value means no context.
	else
	{
		for (let property in object)
		{
			if (property in keys)
			{
				if (isTranslatableString(object[property]))
					if (keys[property])
						object[property] = translateWithContext(keys[property], object[property]);
					else
						object[property] = translate(object[property]);
				else if (object[property] instanceof Object)
					object[property] = translateMessageObject(object[property]);
			}
			else if (object[property] instanceof Object)
				translateObjectKeys(object[property], keys);
		}
	}
}

/**
 * Function is used by the extract-messages tool.
 * So it may only be used on a plain string,
 * it won't have any effect on a calculated string.
 */
function markForTranslation(message)
{
	return message;
}

function markForTranslationWithContext(context, message)
{
	return message;
}

function markForPluralTranslation(singularMessage, pluralMessage, number)
{
	return {
		"message": singularMessage,
		"pluralMessage": pluralMessage,
		"pluralCount": number
	};
}
