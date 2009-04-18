
// "copy" text into the clipboard. replaces previous contents.
extern LibError sys_clipboard_set(const wchar_t* text);

// allow "pasting" from clipboard. returns the current contents if they
// can be represented as text, otherwise 0.
// when it is no longer needed, the returned pointer must be freed via
// sys_clipboard_free. (NB: not necessary if zero, but doesn't hurt)
extern wchar_t* sys_clipboard_get(void);

// frees memory used by <copy>, which must have been returned by
// sys_clipboard_get. see note above.
extern LibError sys_clipboard_free(wchar_t* copy);
