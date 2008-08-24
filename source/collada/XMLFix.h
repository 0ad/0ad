#ifndef XMLFIX_INCLUDED
#define XMLFIX_INCLUDED

/**
 * Fixes some errors in COLLADA XML files that would otherwise prevent
 * FCollada from loading them successfully.
 * 'out' is either a new XML document, which must be freed with xmlFree;
 * otherwise it is equal to 'text' if no changes were made.
 */
void FixBrokenXML(const char* text, const char** out, size_t* outSize);

#endif // XMLFIX_INCLUDED
