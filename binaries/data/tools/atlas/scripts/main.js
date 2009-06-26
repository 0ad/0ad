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
function loadScript(name /*, ...*/)
{
	var filename = getScriptFilename(name);
	var file = new wxFFile(filename.fullPath);
	var script = file.readAll(); // TODO: handle errors
	file.close();

	var script = Atlas.LoadScript(name+'.js', script);

	// Extract the arguments which the function will actually use
	// (Can't use Array.slice since arguments isn't an Array)
	// (Have to do this rather than use arguments.length, since we sometimes
	// pass unused bottomWindows into init and then destroy the window when realising
	// it wasn't used, and then we mustn't send the destroyed window again later)
	var args = [];
	if (script.init)
		for (var i = 1; i < 1+script.init.length; ++i)
			args.push(arguments[i]);
	
	scriptReloader.add(name, args, filename, script);

	if (script.init)
		script.init.apply(null, args);

	return script;
}

/**
 * Helper function for C++ to easily set dot-separated names
 */
function setValue(name, value)
{
	var obj = global;
	var props = name.split(".");
	for (var i = 0; i < props.length-1; ++i)
		obj = obj[props[i]];
	obj[props[props.length-1]] = value;
}

/**
 * Helper function for C++ to easily get dot-separated names
 */
function getValue(name)
{
	var obj = global;
	var props = name.split(".");
	for (var i = 0; i < props.length; ++i)
		obj = obj[props[i]];
	return obj;
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

	return new XML(xml);
}

// Automatically reload scripts from disk when they have been modified
var scriptReloader = {
	timer: new wxTimer(),
	scripts: [], // [ {name, args, filename, mtime, window, script}, ... ]
	notify: function ()
	{
		for each (var script in scriptReloader.scripts)
		{
			var mtime = script.filename.modificationTime;
			if (mtime - script.mtime != 0)
			{
				print('*** Modifications detected - reloading "' + script.name + '"...\n');
				
				script.mtime = mtime;

				if (script.script && script.script.deinit)
					script.script.deinit();

				if (script.name == 'main')
				{
					// Special case for this file to reload itself
					script.script = loadScript(script.name, null);
					// Copy the important state into the new version of this file
					script.script.scriptReloader.scripts = scriptReloader.scripts;
					// Stop this one
					scriptReloader.timer.stop();
				}
				else
				{
					// TODO: know which arguments are really windows that should be regenerated
					for each (var window in script.args)
						window.destroyChildren();
					script.script = loadScript.apply(null, [script.name].concat(script.args));
					for each (var window in script.args)
						window.layout();
				}
			}
		}
	},
	add: function (name, args, filename, script)
	{
		for each (var s in this.scripts)
			if (s.name == name)
				return; // stop if this is already loaded

		this.scripts.push({ name:name, args:args, filename:filename, mtime:filename.modificationTime, script:script });
	}
};
scriptReloader.timer.onNotify = scriptReloader.notify;
scriptReloader.timer.start(1000);
scriptReloader.add('main', null, getScriptFilename('main'));

loadScript('editorstate');

// Export global functions:
global.loadScript = loadScript;
global.loadXML = loadXML;
global.setValue = setValue;
global.getValue = getValue;
