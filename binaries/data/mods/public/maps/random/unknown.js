RMS.LoadLibrary("rmgen");
RMS.LoadLibrary("rmbiome");
RMS.LoadLibrary("the_unknown");

g_PlayerBases = true;
g_AllowNaval = true;

createUnknownMap();
createUnknownObjects();

ExportMap();
