function getScriptFilename(name)
{
	var relativePath = 'tools/atlas/scripts/' + name + '.js';
	var filename = new wxFileName(relativePath, wxPathFormat.UNIX);
	filename.normalize(wxPathNormalize.DOTS | wxPathNormalize.ABSOLUTE | wxPathNormalize.TILDE,
		Atlas.GetDataDirectory()); // equivalent to MakeAbsolute(dir);
	return filename;
}

/**
 * Loads and executes a script from disk. The script runs in a separate scope,
 * so variables and functions declared in it will not be visible outside that file.
 */
function loadScript(name, window)
{
	var filename = getScriptFilename(name);
	var file = new wxFFile(filename.fullPath);
	var script = file.readAll(); // TODO: handle errors
	file.close();

	var script = Atlas.LoadScript(name+'.js', script);
	scriptReloader.add(name, window, filename);
	
	script.init(window); // TODO: use a variable list of arguments
	
	return script;
}

function loadXML(name)
{
	var relativePath = 'tools/atlas/' + name + '.xml';
	var filename = new wxFileName(relativePath, wxPathFormat.UNIX);
	filename.normalize(wxPathNormalize.DOTS | wxPathNormalize.ABSOLUTE | wxPathNormalize.TILDE,
	Atlas.GetDataDirectory()); // equivalent to MakeAbsolute(dir);

	var file = new wxFFile(filename.fullPath);
	var xml = file.readAll(); // TODO: handle errors
	// TODO: complain (or work) nicely if the XML file starts with
	// "<?xml ...?>" (which E4X doesn't like parsing)
	file.close();

	return eval(xml);
}

function init() { /* dummy function to make the script reloader happy */ }

var scriptReloader = {
	timer: new wxTimer(),
	scripts: [], // [ [filename,window], ... ]
	notify: function ()
	{
		for each (var script in scriptReloader.scripts)
		{
			var mtime = script.filename.modificationTime;
			if (mtime - script.mtime != 0)
			{
				print('*** Modifications detected - reloading "' + script.name + '"...\n');
				script.mtime = mtime;
				if (script.name == 'main')
				{
					// Special case for this file to reload itself
					var obj = loadScript(script.name, null);
					// Copy the important state into the new version of this file
					obj.scriptReloader.scripts = scriptReloader.scripts;
					// Stop this one
					scriptReloader.timer.stop();
				}
				else
				{
					script.window.destroyChildren();
					loadScript(script.name, script.window);
					script.window.layout();
				}
			}
		}
	},
	add: function (name, window, filename)
	{
		for each (var script in this.scripts)
			if (script.name == name)
				return; // stop if this is already loaded
		this.scripts.push({ name:name, window:window, filename:filename, mtime:filename.modificationTime });
	}
};
scriptReloader.timer.onNotify = scriptReloader.notify;
scriptReloader.timer.start(1000);
scriptReloader.add('main', null, getScriptFilename('main'));

// Export global functions:
global.loadScript = loadScript;
global.loadXML = loadXML;
