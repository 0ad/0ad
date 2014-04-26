var g_translations = {};
var g_pluralTranslations = {};
var g_translationsWithContext = {};
var g_pluralTranslationsWithContext = {};

// Checks if the specified variable is a string, and if it is, it checks that it
// is not empty.
function isNonEmptyString(variable)
{
	if (typeof variable != "string")
		return false;
	else if (variable.trim())
		return true;
	else
		return false;
}

// Translates the specified English message into the current language.
//
// This function relies on the g_translations cache when possible. You should use this function instead of
// Engine.Translate() whenever you can to minimize the number of C++ calls and string conversions involved.
function translate(message)
{
	var translation = g_translations[message];
	if (!translation)
		return g_translations[message] = Engine.Translate(message);
	return translation;
}


// Translates the specified English message into the current language for the specified number.
//
// This function relies on the g_pluralTranslations cache when possible. You should use this function instead of
// Engine.TranslatePlural() whenever you can to minimize the number of C++ calls and string conversions involved.
function translatePlural(singularMessage, pluralMessage, number)
{
	var translation = g_pluralTranslations[singularMessage];
	if (!translation)
		g_pluralTranslations[singularMessage] = {};

	var pluralTranslation = g_pluralTranslations[singularMessage][number];
	if (!pluralTranslation)
		return g_pluralTranslations[singularMessage][number] = Engine.TranslatePlural(singularMessage, pluralMessage, number);

	return pluralTranslation;
}


// Translates the specified English message into the current language for the specified context.
//
// This function relies on the g_translationsWithContext cache when possible. You should use this function instead of
// Engine.TranslateWithContext() whenever you can to minimize the number of C++ calls and string conversions involved.
function translateWithContext(context, message)
{
	var translationContext = g_translationsWithContext[context];
	if (!translationContext)
		g_translationsWithContext[context] = {}

	var translationWithContext = g_translationsWithContext[context][message];
	if (!translationWithContext)
		return g_translationsWithContext[context][message] = Engine.TranslateWithContext(context, message);

	return translationWithContext;
}


// Translates the specified English message into the current language for the specified context and number.
//
// This function relies on the g_pluralTranslationsWithContext cache when possible. You should use this function instead of
// Engine.TranslatePluralWithContext() whenever you can to minimize the number of C++ calls and string conversions involved.
function translatePluralWithContext(context, singularMessage, pluralMessage, number)
{
	var translationContext = g_pluralTranslationsWithContext[context];
	if (!translationContext)
		g_pluralTranslationsWithContext[context] = {};

	var translationWithContext = g_pluralTranslationsWithContext[context][singularMessage];
	if (!translationWithContext)
		g_pluralTranslationsWithContext[context][singularMessage] = {};

	var pluralTranslationWithContext = g_pluralTranslationsWithContext[context][singularMessage][number];
	if (!pluralTranslationWithContext)
		return g_pluralTranslationsWithContext[context][singularMessage][number] = Engine.TranslatePluralWithContext(context, singularMessage, pluralMessage, number);

	return pluralTranslationWithContext;
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
	// the translation function
	var trans = translate;
	if (object.context)
		trans = function(msg) { return translateWithContext(object.context, msg);};

	if (object.message)
		object = trans(object.message);
	else if (object.list)
	{
		var translatedList = object.list.map(trans);
		object = translatedList.join(translateWithContext("enumeration", ", "));
	}

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
function translateObjectKeys(object, keys) {
	// If ‘keys’ is an array, simply translate.
	if (keys instanceof Array)
	{
		for (var property in object)
		{
			if (keys.indexOf(property) > -1)
			{
				if (isNonEmptyString(object[property]))
					object[property] = translate(object[property]);
				else if (object[property] instanceof Object)
				{
					object[property] = translateMessageObject(object[property]);
				}
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
		for (var property in object)
		{
			if (property in keys)
			{
				if (isNonEmptyString(object[property]))
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

function markForTranslation(message) {
	return message;
}

function markForTranslationWithContext(context, message) {
	return message;
}
	
