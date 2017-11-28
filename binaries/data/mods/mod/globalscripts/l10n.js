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
