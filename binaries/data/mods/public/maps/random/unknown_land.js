Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");
Engine.LoadLibrary("rmbiome");
Engine.LoadLibrary("the_unknown");

g_AllowNaval = false;

createUnknownMap();

g_Map.ExportMap();
