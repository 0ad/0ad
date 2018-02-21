Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");
Engine.LoadLibrary("rmbiome");
Engine.LoadLibrary("the_unknown");

g_AllowNaval = true;

createUnknownMap();

g_Map.ExportMap();
