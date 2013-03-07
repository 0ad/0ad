var server_url = 'http://127.0.0.1:9000';
var g_Data = {};
g_Data.fileData = {};

function debug(message) {
    if (window.console) {
        console.log(message);
    }
}

function send(path) {
    $.get(server_url + path).complete(function (response) {
        if (response.status == 200) {

        } else {
            debug('FAILED : ' + path);
            alert("Failed to toggle breakpoint.\nIs the game running in debug mode?");
        }
    });
}

function updateFileList() {
    $.get(server_url + '/EnumVfsJSFiles').complete(function (response) {
        if (response.status == 200) {

            var files = jQuery.parseJSON(response.responseText);
            files.sort();
            g_Data.fileList = files;
            updateFileListFromData();

        } else {
            debug('FAILED : updateFileList');
            alert("Failed to update file list.");
        }
    });
}

function updateFileListFromData() {
	var files = g_Data.fileList;
	var search = $('#file_search').val().toLowerCase();
    var items = [];
    for (var i = 0; i < files.length; i++) {
        var file = files[i];
        
        if (search != "") {
        	if(file.toLowerCase().indexOf(search) == -1) {
        		continue;
        	}
        }
        
        var item = {
            file_path: file
        };

        items.push(item);
    }

    $('#files').datagrid({
        data: items
    });
}

function openFile(path) {
	openFileAtLine(path, 0);
}

function openFileAtLine(path, line) {
	
	if (g_Data.file == path) {
		// Already at this file
		editor.gotoLine(line);
	    updateDebugMarker();
		return;		
	}

    $.get(server_url + '/GetFile?filename=' + path).complete(function (response) {
        if (response.status == 200) {
        
        	$('#filePanel').panel({  
    			title: 'File: ' + path,
			}); 

			g_Data.file = path;
			
			var file = response.responseText;
			var fileData = getFileData(path);
			
            editor.selection.clearSelection();
            editor.session.clearBreakpoints();
            editor.setValue(file);
            editor.gotoLine(line);
            
            for (var i = 0; i < fileData.breakpoints.length; i++) {
            	var breakLine = fileData.breakpoints[i];
            	editor.session.setBreakpoint(breakLine - 1);
            }
            
			updateDebugMarker();

        } else {
            debug('FAILED : openFile');
            alert("Failed to open file.");
        }
    });
}

function updateDebugMarker() {
    if (g_Data.debugMarker != undefined) {
    	editor.session.removeMarker(g_Data.debugMarker);
    	g_Data.debugMarker = undefined;
    }
    
    if (g_Data.debugFile == g_Data.file && g_Data.debugLine != undefined) {
        var Range = ace.require('ace/range').Range;
    	g_Data.debugMarker = editor.session.addMarker(new Range(g_Data.debugLine-1, 0, g_Data.debugLine, 0), "breakhighlight", "line");
    }
}

function getFileData(file) {
	var fileData = g_Data.fileData[file];
	
	if (fileData == undefined) {
		fileData = {};
		fileData.breakpoints = [];
		g_Data.fileData[file] = fileData;
	}
	
	return fileData;
}

function toggleBreakpointAt(line, onComplete) {
	
	var addr = server_url + '/ToggleBreakpoint?filename=' + g_Data.file + '&line=' + line;
	$.get(addr).complete(function (response) {
        if (response.status == 200) {
        
        	var fileData = getFileData(g_Data.file);
			var index = fileData.breakpoints.indexOf(line);
			
			if (index >= 0) {
				fileData.breakpoints.splice(index, 1);
			} else { 
				fileData.breakpoints.push(line);
			}	
			
	        if(onComplete != undefined) {
	        	onComplete(true, index < 0);
	        }

        } else {  
            alert("Failed to toggle breakpoint.\nIs the game running in debug mode?");
            
            if(onComplete != undefined) {
	        	onComplete(false, false);
	        }
        }
    });
}

function stack(threadId) {
	updateStack(threadId, 0);
}


function updateStack(threadId, nestingLevel) {
    $.get(server_url + '/GetStackFrame?nestingLevel=' + nestingLevel + '&threadDebuggerID=' + threadId).complete(function (response) {
        if (response.status == 200) {
        
            var data = response.responseText;
            var values = jQuery.parseJSON(response.responseText);
            
            $('#vars').treegrid({
		       data: []
		    });

            addLocalValues(null, values);

            debug('SUCCESS : openFile');
        } else {
            debug('FAILED : openFile');
            alert("Failed to open file.");
        }
    });
}


function updateThreads() {
    $.get(server_url + '/GetThreadDebuggerStatus').complete(function (response) {
        if (response.status == 200) {

            var data = response.responseText;
            
            if (g_Data.lastThreadData == data) {
            	// Nothing changed
            	return;
            }
            
            g_Data.lastThreadData = data;

            var values = jQuery.parseJSON(data);

            var items = [];
            var threads = {};

            for (var i = 0; i < values.length; i++) {
                var thisValue = values[i];
                
                var thread = {
                    id: thisValue.ThreadDebuggerID,
                    scriptInterface: thisValue.ScriptInterfaceName,
                    inBreak: thisValue.ThreadInBreak,
                    data: thisValue
                };

                if (thread.inBreak) {
                    thread.breakFile = thisValue.BreakFileName;
                    thread.breakLine = thisValue.BreakLine;
                }
				
				threads[thread.id] = thread;
                items.push(thread);
            }
            
            g_Data.threads = threads;
            
            updateDebuggingThread();
            updateDebugMarker();

            $('#threads').datagrid({
                data: items
            });

        } else {
            debug('Failed to update threads');
        }
    });
}

function updateCallStack() {
    $.get(server_url + '/GetAllCallstacks').complete(function (response) {
        if (response.status == 200) {

            var data = response.responseText;

            var values = jQuery.parseJSON(data);
            
            values = values.CallStacks
            
            var locations = undefined;
            for (var i = 0; i < values.length; i++) {
                
                if(values[i].ThreadDebuggerID == g_Data.debugThread) {
                	locations = values[i].CallStack;
                	break;
                }
            }

            var items = [];

			if(locations != undefined) {
	            for (var i = 0; i < locations.length; i++) {
	                
	                var item = {
	                    id: i,
	                    location: locations[i]
	                };
	
	                items.push(item);
	            }
            }

            $('#call_stack').datagrid({
                data: items
            });
            

        } else {
            debug('Failed to update call stack');
        }
    });
}

function updateDebuggingThread() {
	
	var debugThread;
	
    if (g_Data.debugThread) {
    	var thread = g_Data.threads[g_Data.debugThread];
    	if (thread && thread.inBreak) {
    		debugThread = thread;
    	}
    }
    
    if(!debugThread) {
    	for (threadId in g_Data.threads) {
    		var thread = g_Data.threads[threadId];
    		if (thread.inBreak) {
    			debugThread = thread;
    			break;
    		}
    	}
    }

	if (debugThread == undefined) {
		delete g_Data.debugThread;
		delete g_Data.debugFile;
		delete g_Data.debugLine;
		return;
	}
	
	if (g_Data.debugThread != debugThread.id || 
		g_Data.debugFile != debugThread.breakFile ||
		g_Data.debugLine != debugThread.breakLine) {
		
		beginThreadDebug(debugThread.id);
	}
}


function beginThreadDebug(id) {
	var thread = g_Data.threads[id];
	g_Data.debugThread = id;
	
	delete g_Data.debugFile;
	delete g_Data.debugLine;
	
	if (!thread.inBreak) {
		updateDebugMarker();
		return;
	}
	
	g_Data.debugFile = thread.breakFile;
	g_Data.debugLine = thread.breakLine;
	
	openFileAtLine(thread.breakFile, thread.breakLine);
	updateCallStack();
	stack(thread.id);
}



function addLocalValues(parent, obj) {
    var num = 0;
    var items = [];

    for (key in obj) {
        var id = (parent == null) ? num : parent + "_" + num;
        var value = obj[key];

        var item = {
            id: id,
            name: key,
            value: value,
            iconCls: 'icon-blank'
        };

        if (typeof (value) == "object") {
            item.state = "closed";
            item.children = [{
                id: id + "_SUB",
                name: "-",
                value: "-",
                iconCls: 'icon-blank'
            }];
        }

        items.push(item)
        num++;
    }
    
	items.sort(function (a, b){
	  var aName = a.name.toLowerCase();
	  var bName = b.name.toLowerCase(); 
	  return ((aName < bName) ? -1 : ((aName > bName) ? 1 : 0));
	});

    $('#vars').treegrid('append', {
        parent: parent,
        data: items
    });
}

$(document).ready(function () {

	var debugUpdate = 150;

    $('a#step').click(function () {
        send('/Step?threadDebuggerID=' + g_Data.debugThread);
        setTimeout(function(){ updateThreads(); }, debugUpdate);
        return false;
    });
    
    $('a#step_into').click(function () {
        send('/StepInto?threadDebuggerID=' + g_Data.debugThread);
        setTimeout(function(){ updateThreads(); }, debugUpdate);
        return false;
    });
    
    $('a#step_out').click(function () {
        send('/StepOut?threadDebuggerID=' + g_Data.debugThread);
        setTimeout(function(){ updateThreads(); }, debugUpdate);
        return false;
    });

    $('a#continue').click(function () {
        send('/Continue?threadDebuggerID=' + g_Data.debugThread);
        setTimeout(function(){ updateThreads(); }, debugUpdate);
        return false;
    });

    $('#threads').datagrid({
		onDblClickRow: function(rowIndex, rowData) {
			clearSelection();
			beginThreadDebug(rowData.id);
			return false;
		}
	});
	
	$('#call_stack').datagrid({
		onDblClickRow: function(rowIndex, rowData) {
			clearSelection();
			updateStack(g_Data.debugThread, rowData.id);
			return false;
		}
	});
	
	$('#files').datagrid({
		onSelect: function(rowIndex, rowData) {
			openFile(rowData.file_path);
			return false;
		}
	});
	
	$('#filePanel').panel({
		onResize: function(width, height) {
			editor.resize();
			return false;
		}
	});	
	
	$('#file_search').keyup(function(e) {
		updateFileListFromData();
		$('#file_search').focus();
		return false;
	});
    
    $('#vars').treegrid({
        onExpand: function (row) {
            var children = $('#vars').treegrid('getChildren', row.id);
            for (var i = 0; i < children.length; i++) {
                if (children[i]._parentId == row.id) {
                    $('#vars').treegrid('remove', children[i].id);
                }
            }
            addLocalValues(row.id, row.value);
        },
        /*
        onCollapse: function(row){
        	var children = $('#vars').treegrid('getChildren', row.id);
        	for (var i = 0; i < children.length; i++) {
        		$('#vars').treegrid('remove', children[i].id);
			}
        	
        	$('#vars').treegrid('append',{
				parent: row.id,
				data: [{
					id: row.id + "_SUB",
					name: "-",
					value: "-",
					iconCls: 'icon-blank'
				}]
			});
        }
        */
    });

    updateFileList();
    updateThreads();

	setInterval(function(){ updateThreads(); }, 1000);
	
	editor.resize();
});

function clearSelection() {
    if(document.selection && document.selection.empty) {
        document.selection.empty();
    } else if(window.getSelection) {
        var sel = window.getSelection();
        sel.removeAllRanges();
    }
}

function threadRowStyler(index,rowData){  
	if (rowData.id == g_Data.debugThread){
		var thread = g_Data.threads[g_Data.debugThread];
		if (thread != undefined && thread.inBreak) {
	    	return 'background-color:#ffee00;color:black;';  
	    }
	}  
}