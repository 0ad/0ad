function setupSimTest(window)
{
	var state = 'inactive'; // inactive, playing
	var speed = 0;
	
	function play(newSpeed)
	{
		if (state == 'inactive')
			Atlas.Message.SimStateSave('default', true);
		Atlas.Message.SimPlay(newSpeed);
		state = 'playing';
		speed = newSpeed;
		updateEnableStatus();
	}

	function reset()
	{
		Atlas.Message.SimPlay(0);
		Atlas.Message.SimStateRestore('default');
		state = 'inactive';
		speed = 0;
		updateEnableStatus();
	}
	
	function updateEnableStatus()
	{
		for (var i in buttons)
			buttons[i][3].enable = buttons[i][2]();
	}

	var speeds = [ 1/8, 1, 8 ];
	
	var buttons = [
		/* button label; click command; active condition */
		[ 'Play', function () { play(speeds[1]) }, function () { return !(state == 'playing' && speed == speeds[1]) } ],
		[ 'Fast', function () { play(speeds[2]) }, function () { return !(state == 'playing' && speed == speeds[2]) } ],
		[ 'Slow', function () { play(speeds[0]) }, function () { return !(state == 'playing' && speed == speeds[0]) } ],
		[ 'Pause', function () { play(0) }, function () { return state == 'playing' && speed != 0 } ],
		[ 'Reset', function () { reset() }, function () { return state != 'inactive' } ],
	];
	
	var sizer = new wxStaticBoxSizer(new wxStaticBox(window, -1, 'Simulation test'), wxOrientation.HORIZONTAL);
	for each (var b in buttons)
	{
		var button = new wxButton(window, -1, b[0]);
		button.onClicked = b[1];
		sizer.add(button, 1);
		b[3] = button;
	}
	updateEnableStatus();
	window.sizer.add(sizer, 0, wxStretch.EXPAND | wxDirection.LEFT|wxDirection.RIGHT, 2);
}

function generateRMS(name)
{
	var cwd = wxFileName.cwd;
	wxFileName.cwd = Atlas.GetDataDirectory();
	var ret = wxExecute([ 'rmgen.exe', name, '_atlasrm' ], wxExecFlag.SYNC); // TODO: portability
	wxFileName.cwd = cwd;
	if (ret == 0)
		Atlas.Message.LoadMap('_atlasrm.pmp');
	else
		wxMessageBox('Failed to run rmgen (error code: '+ret+')');
}

function init(window)
{
	window.sizer = new wxBoxSizer(wxOrientation.VERTICAL);
	
	var button = new wxButton(window, -1, 'Generate empty map');
	button.onClicked = function () { Atlas.Message.GenerateMap(9); };
	window.sizer.add(button, 0, wxDirection.ALL, 2);
	
	var sizer = new wxBoxSizer(wxOrientation.HORIZONTAL);
	var rmsFilename = new wxTextCtrl(window, -1, 'cantabrian_highlands');
	var rmsButton = new wxButton(window, -1, 'Run RMS');
	rmsButton.onClicked = function () { generateRMS(rmsFilename.value) }
	sizer.add(rmsFilename, 1);
	sizer.add(rmsButton, 0);
	window.sizer.add(sizer, 0, wxStretch.EXPAND | wxDirection.ALL, 2);
	
	setupSimTest(window);
}
