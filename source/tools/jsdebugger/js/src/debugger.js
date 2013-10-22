var server_url = 'http://127.0.0.1:9000';
var g_Data = {};
var g_ExpandedNodes = {};
var g_RootNodeNames = [ 'locals', 'this', 'global' ];
g_Data.fileData = {};

function debug(message) 
{
	if (window.console) 
	{
		console.log(message);
	}
}

function send(path) 
{
	$.get(server_url + path).complete(function(response) 
	{
		if (response.status == 200) 
		{ 
		} 
		else 
		{
			debug('FAILED : ' + path);
			alert("Failed to toggle breakpoint.\nIs the game running in debug mode?");
		}
	});
}

function updateFileList() 
{
	$.get(server_url + '/EnumVfsJSFiles').complete(function (response) 
	{
		if (response.status == 200) 
		{
			var files = jQuery.parseJSON(response.responseText);
			files.sort();
			g_Data.fileList = files;
			updateFileListFromData();
			} 
		else 
		{
			debug('FAILED : updateFileList');
			alert("Failed to update file list.");
		}
	});
}

function updateFileListFromData() 
{
	var files = g_Data.fileList;
	var search = $('#file_search').val().toLowerCase();
	var items = [];
	for (var i = 0; i < files.length; i++) 
	{
		var file = files[i];

		if (search != "") 
		{
			if(file.toLowerCase().indexOf(search) == -1) 
			{
				continue;
			}
		}
	
		var item = { file_path: file };

		items.push(item);
	}

	$('#files').datagrid( { data: items } );
}

function openFile(path) 
{
	openFileAtLine(path, 0);
}

function openFileAtLine(path, line) 
{
	if (g_Data.file == path) 
	{
		// Already at this file
		editor.gotoLine(line);
		updateDebugMarker();
		return;		
	}

	$.get(server_url + '/GetFile?filename=' + path).complete(function (response) 
	{
		if (response.status == 200) 
		{   
			$('#filePanel').panel( { title: 'File: ' + path, } ); 

			g_Data.file = path;
			
			var file = response.responseText;
			var fileData = getFileData(path);
			
			editor.selection.clearSelection();
			editor.session.clearBreakpoints();
			editor.setValue(file);
			editor.gotoLine(line);

			for (var i = 0; i < fileData.breakpoints.length; i++) 
			{
				var breakLine = fileData.breakpoints[i];
				editor.session.setBreakpoint(breakLine - 1);
			}
			
			updateDebugMarker();
		} 
		else 
		{
			debug('FAILED : openFile');
			alert("Failed to open file.");
		}
	});
}

function updateDebugMarker() 
{
	if (g_Data.debugMarker != undefined) 
	{
		editor.session.removeMarker(g_Data.debugMarker);
		g_Data.debugMarker = undefined;
	}

	if (g_Data.debugFile == g_Data.file && g_Data.debugLine != undefined) 
	{
		var Range = ace.require('ace/range').Range;
		g_Data.debugMarker = editor.session.addMarker(new Range(g_Data.debugLine-1, 0, g_Data.debugLine, 0), "breakhighlight", "line");
	}
}

function getFileData(file)
{
	var fileData = g_Data.fileData[file];
	
	if (fileData == undefined)
	{
		fileData = {};
		fileData.breakpoints = [];
		g_Data.fileData[file] = fileData;
	}
	
	return fileData;
}

function toggleBreakpointAt(line, onComplete) 
{
	var addr = server_url + '/ToggleBreakpoint?filename=' + g_Data.file + '&line=' + line;
	$.get(addr).complete(function (response) 
	{
		if (response.status == 200) {
		
			var fileData = getFileData(g_Data.file);
			var index = fileData.breakpoints.indexOf(line);
			
			if (index >= 0) 
			{
				fileData.breakpoints.splice(index, 1);
			}
			else 
			{ 
				fileData.breakpoints.push(line);
			}
			
			if(onComplete != undefined)
			{
				onComplete(true, index < 0);
			}

		} 
		else 
		{  
			alert("Failed to toggle breakpoint.\nIs the game running in debug mode?");
			
			if(onComplete != undefined) 
			{
				onComplete(false, false);
			}
		}
	});
}

function stack(threadId) 
{
	updateStack(threadId, 0);
}

function getCurrentGlobalObject (threadId) 
{
	$.ajax(server_url + '/GetCurrentGlobalObject?threadDebuggerID=' + threadId,
	{
		type: 'GET', 
		dataType : 'json',
		success: function(data, textStatus, jqXHR)
		{
			addWatchValues('global', jqXHR);
		},
		error: function(jqXHR, textStatus, errorThrown)
		{
			debug('ERROR : getCurrentGlobalObject: ' + errorThrown);
			alert('getCurrentGlobalObject failed:\n' + errorThrown);
		}
	});
}

function getStackFrame (threadId, nestingLevel) 
{
	$.ajax(server_url + '/GetStackFrame?nestingLevel=' + nestingLevel + '&threadDebuggerID=' + threadId,
	{
		type: 'GET', 
		dataType : 'json',
		success: function(data, textStatus, jqXHR)
		{
			addWatchValues('locals', jqXHR);
		},
		error: function(jqXHR, textStatus, errorThrown)
		{
			debug('ERROR : getStackFrame: ' + errorThrown);
			alert('getStackFrame failed:\n' + errorThrown);
		}
	});
}
 
function getStackFrameThis (threadId, nestingLevel) 
{
	$.ajax(server_url + '/GetStackFrameThis?nestingLevel=' + nestingLevel + '&threadDebuggerID=' + threadId,
	{
		type: 'GET', 
		dataType : 'json',
		success: function(data, textStatus, jqXHR)
		{
			addWatchValues('this', jqXHR);
		},
		error: function(jqXHR, textStatus, errorThrown)
		{
			debug('ERROR : getStackFrameThis: ' + errorThrown);
			alert('getStackFrameThis failed:\n' + errorThrown);
		}
	});
}

// Creates a root node and fill it with a dummy object 
function recreateRootNode(name, expand)
{
	// remove the root node's children
	// Don't remove the root node itself because it needs to stay to show a loading-icon
	var node = $('#vars').treegrid('find', name);
	if (node)
	{ 
		// remove any kind off "dummy-children" we added
		// TODO: used twice, make a function
		var children = $('#vars').treegrid('getChildren', node.name);
		for (var i = 0; i < children.length; i++) 
		{
			if (children[i]._parentId == node.name) 
			{
				$('#vars').treegrid('remove', children[i].id);
			}
		}
	}

	
	// if the root node was expanded or should be expanded, load it.
	if ( (g_ExpandedNodes !== undefined && g_ExpandedNodes[name] !== undefined) || expand == true)
	{
		var id = 0;
		var row = $('#call_stack').datagrid('getSelected');
		if (row)
		{
			id = row.id;	
		}
		$('#vars').treegrid('update',{
			id: name,
			row: { iconCls: 'icon-loading' }
		});
		
		
		if (name == 'locals')
		{
			getStackFrame(g_Data.debugThread, id);
		}
		else if (name == 'this')
		{
			getStackFrameThis(g_Data.debugThread, id);
		}
		else if (name == 'global')
		{
			getCurrentGlobalObject(g_Data.debugThread);
		}
	}
	else
	{
		var dummy = { 'dummy' : 'dummy' };
		var node = $('#vars').treegrid('find', name);
		if (node)
		{ 
			// add the dummy object to make the root node expandable
			addLocalValues(name, dummy);
			// Adding the subnode automatically expands the root node, so collapse it agiain
			$('#vars').treegrid('collapse', name);
		}
		else // no root node yet
		{
			// create a root node with a dummy child object	
			var rootNode = {};
			rootNode[name] = dummy;
			addLocalValues(null, rootNode); 
		}
	}	
}

function updateStack(threadId, nestingLevel) 
{
	for (idx in g_RootNodeNames)
	{
		recreateRootNode(g_RootNodeNames[idx], false)
	}	
}

function addWatchValues(name, response) 
{
	
	if (response.status == 200) 
	{
		var values = jQuery.parseJSON(response.responseText);
		addLocalValues(name, values);
		
		// We have loaded the subnodes for a root node (in contrast to just having a dummy-object below the root-node).
		// Now expand any subnodes that were previously expanded
		for(var nodeName in g_ExpandedNodes)
		{
			var node = $('#vars').treegrid('find', nodeName);
			if (node)
			{
				if ($.inArray(nodeName, g_RootNodeNames) == -1)
				{
					$('#vars').treegrid('toggle', nodeName);
				}
			}			
		}
		
		debug('SUCCESS : addWatchValues');
	} 
	else 
	{
		debug('FAILED : addWatchValues');
		alert("Failed to add watch values.");
	}
	
	$('#vars').treegrid('update',{
		id: name,
		row: { iconCls: 'icon-blank' }
	});
}


function updateThreads() 
{
	$.get(server_url + '/GetThreadDebuggerStatus').complete(function (response) 
	{
		if (response.status == 200) 
		{
			var data = response.responseText;
			
			if (g_Data.lastThreadData == data) 
			{
				// Nothing changed
				return;
			}
			
			g_Data.lastThreadData = data;

			var values = jQuery.parseJSON(data);

			var items = [];
			var threads = {};

			var anyThreadInBreak = false;
			for (var i = 0; i < values.length; i++) 
			{
				var thisValue = values[i];
				
				var thread = 
				{
					id: thisValue.ThreadDebuggerID,
					scriptInterface: thisValue.ScriptInterfaceName,
					inBreak: thisValue.ThreadInBreak,
					data: thisValue
				};

				if (thread.inBreak) 
				{
					thread.breakFile = thisValue.BreakFileName;
					thread.breakLine = thisValue.BreakLine;
					anyThreadInBreak = true;
				}
				
				threads[thread.id] = thread;
				items.push(thread);
			}
			
			g_Data.threads = threads;
			
			updateDebuggingThread();
			updateDebugMarker();
			
			if (!anyThreadInBreak)
			{
				updateCallStack();
				updateStack();
			}

			$('#threads').datagrid( { data: items });
		} 
		else 
		{
			debug('Failed to update threads');
		}
	});
}

function updateCallStack() 
{
	$.get(server_url + '/GetAllCallstacks').complete(function (response) 
	{
		if (response.status == 200) 
		{
			var data = response.responseText;
			var values = jQuery.parseJSON(data);
			
			var locations = undefined;
			for (var i = 0; i < values.length; i++) 
			{
				
				if(values[i].ThreadDebuggerID == g_Data.debugThread) 
				{
					locations = values[i].CallStack;
					break;
				}
			}

			var items = [];

			if(locations != undefined) 
			{
				for (var i = 0; i < locations.length; i++) 
				{
					var item = 
					{
						id: i,
						location: locations[i]
					};
					items.push(item);
				}
			}
			$('#call_stack').datagrid( { data: items } );
		} 
		else 
		{
			debug('Failed to update call stack');
		}
	});
}

function updateDebuggingThread() 
{
	var debugThread;
	
	if (g_Data.debugThread) 
	{
		var thread = g_Data.threads[g_Data.debugThread];
		if (thread && thread.inBreak) 
		{
			debugThread = thread;
		}
	}
	
	if(!debugThread) 
	{
		for (threadId in g_Data.threads) 
		{
			var thread = g_Data.threads[threadId];
			if (thread.inBreak) 
			{
				debugThread = thread;
				break;
			}
		}
	}

	if (debugThread == undefined) 
	{
		delete g_Data.debugThread;
		delete g_Data.debugFile;
		delete g_Data.debugLine;
		return;
	}
	
	if (g_Data.debugThread != debugThread.id || 
	     g_Data.debugFile != debugThread.breakFile ||
	     g_Data.debugLine != debugThread.breakLine) 
	{
		beginThreadDebug(debugThread.id);
	}
}


function beginThreadDebug(id) 
{
	var thread = g_Data.threads[id];
	g_Data.debugThread = id;
	
	delete g_Data.debugFile;
	delete g_Data.debugLine;
	
	if (!thread.inBreak) 
	{
		updateDebugMarker();
		return;
	}
	
	g_Data.debugFile = thread.breakFile;
	g_Data.debugLine = thread.breakLine;
	
	openFileAtLine(thread.breakFile, thread.breakLine);
	updateCallStack();
	stack(thread.id);
}



function addLocalValues(parent, obj) 
{
	var items = [];

	for (key in obj) 
	{
		var id = (parent == null) ? key : parent + "_" + key;
		var value = obj[key];

		var item = {
			id: id,
			name: key,
			value: value,
			iconCls: 'icon-blank'
		};

		if (typeof (value) == "object") 
		{
			item.state = "closed";
			item.children = [{
				id: id + "_SUB",
				name: "-",
				value: "-",
				iconCls: 'icon-blank'
			}];
		}

		items.push(item)
	}
	
	items.sort(function (a, b)
	{
		var aName = a.name.toLowerCase();
		var bName = b.name.toLowerCase(); 
		return ((aName < bName) ? -1 : ((aName > bName) ? 1 : 0));
	});

	$('#vars').treegrid('append', 
	{
		parent: parent,
		data: items
	});
}

$(document).ready(function () 
{
	var debugUpdate = 150;
	var keydownToClick = { //calls the appropriate click handler on a keydown
		116: $('a#step'),
		117: $('a#step_into'),
		118: $('a#step_out'),
		119: $('a#continue'),
		120: $('a#continue_thread'),
		121: $('a#break'),
	}; 

	$('a#step').click(function () 
	{
		send('/Step?threadDebuggerID=' + g_Data.debugThread);
		setTimeout(function(){ updateThreads(); }, debugUpdate);
		return false;
	});
	
	$('a#step_into').click(function () 
	{
		send('/StepInto?threadDebuggerID=' + g_Data.debugThread);
		setTimeout(function(){ updateThreads(); }, debugUpdate);
		return false;
	});
	
	$('a#step_out').click(function () 
	{
		send('/StepOut?threadDebuggerID=' + g_Data.debugThread);
		setTimeout(function(){ updateThreads(); }, debugUpdate);
		return false;
	});

	$('a#continue').click(function () 
	{
		send('/Continue?threadDebuggerID=' + 0);
		setTimeout(function(){ updateThreads(); }, debugUpdate);
		return false;
	});
	
	$('a#continue_thread').click(function () 
	{
		send('/Continue?threadDebuggerID=' + g_Data.debugThread);
		setTimeout(function(){ updateThreads(); }, debugUpdate);
		return false;
	});
	
	$('a#break').click(function () 
	{
		send('/Break');
		setTimeout(function(){ updateThreads(); }, debugUpdate);
		return false;
	});

	$(document).bind('keydown', function(e) {
		if (keydownToClick[e.which] != undefined) {
			keydownToClick[e.which].click();
			return false;
		}
	});

	$('#threads').datagrid(
	{
		onDblClickRow: function(rowIndex, rowData) {
			clearSelection();
			beginThreadDebug(rowData.id);
			return false;
		}
	});
	
	$('#call_stack').datagrid(
	{
		onDblClickRow: function(rowIndex, rowData) {
			clearSelection();
			updateStack(g_Data.debugThread, rowData.id);
			return false;
		}
	});
	
	$('#files').datagrid(
	{
		onSelect: function(rowIndex, rowData) 
		{
			openFile(rowData.file_path);
			return false;
		}
	});
	
	$('#filePanel').panel(
	{
		onResize: function(width, height) 
		{
			editor.resize();
			return false;
		}
	});	
	
	$('#file_search').keyup(function(e) 
	{
		updateFileListFromData();
		$('#file_search').focus();
		return false;
	});
	
	$('#vars').treegrid(
	{
		onExpand: function (row)
		{
			// remove any kind off "dummy-children" we added
			var children = $('#vars').treegrid('getChildren', row.id);
			for (var i = 0; i < children.length; i++) 
			{
				if (children[i]._parentId == row.id) 
				{
					$('#vars').treegrid('remove', children[i].id);
				}
			}
			
			
			if ($.inArray(row.id, g_RootNodeNames) !== -1)
			{
				recreateRootNode(row.id, true);
			}
			else
			{			
				addLocalValues(row.id, row.value);
			}
			
			g_ExpandedNodes[row.id] = true;

		},
		onCollapse: function(row)
		{
			delete g_ExpandedNodes[row.id];
		}
	});

	updateFileList();
	updateThreads();
	setInterval(function(){ updateThreads(); }, 1000);
	editor.resize();
});

function clearSelection() 
{
	if(document.selection && document.selection.empty) 
	{
		document.selection.empty();
	} 
	else if(window.getSelection) 
	{
		var sel = window.getSelection();
		sel.removeAllRanges();
	}
}

function threadRowStyler(index,rowData)
{
	if (rowData.id == g_Data.debugThread)
	{
		var thread = g_Data.threads[g_Data.debugThread];
		if (thread != undefined && thread.inBreak) 
		{
			return 'background-color:#ffee00;color:black;';  
		}
	}  
}
