/*
Functions defined here are global, and accessible by any code.
*/

/**
 * Loads and executes a script from disk. The script runs in a separate scope,
 * so variables and functions declared in it will not be visible outside that file.
 */
function loadScript(name, window)
{
	var relativePath = 'tools/atlas/scripts/' + name + '.js';
	var filename = new wxFileName(relativePath, wxPathFormat.UNIX);
	filename.normalize(wxPathNormalize.DOTS | wxPathNormalize.ABSOLUTE | wxPathNormalize.TILDE,
		Atlas.GetDataDirectory()); // equivalent to MakeAbsolute(dir);

	var file = new wxFFile(filename.fullPath);
	var script = file.readAll(); // TODO: handle errors
	file.close();

	var script = Atlas.LoadScript(name+'.js', script);
	script.init(window); // TODO: use a variable list of arguments
}
