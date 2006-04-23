#ifndef __RMGEN_H__
#define __RMGEN_H__

extern JSRuntime *rt;
extern JSContext *cx;
extern JSObject *global;

extern class Map* theMap;   // ugly

// Utility functions
void Shutdown(int status);
char* ValToString(jsval val);
jsval NewJSString(const std::string& str);

#endif