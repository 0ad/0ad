/**
 * Contents of data.json
 */
var replayData;

/**
 * Number of turns between two saved profiler snapshots.
 * Keep in sync with Replay.cpp.
 */
var PROFILE_TURN_INTERVAL = 20;

/**
 * These columns are never displayed.
 */
var filteredColumns = [
	"sim update", // indistringuishable from Total
	"unlogged" // Bogus
];


/**
 * Determines how the the different graphs are formatted.
 */
var graphFormat = {
	"time": {
		"axisTitle": "Time per frame",
		"unit": "milliseconds",
		"digits": 2,
		"scale": 1,
	},
	"bytes": {
		"axisTitle": "Memory",
		"unit": "Megabytes",
		"digits": 2,
		"scale": 1 / 1024 / 1024,
		"isColumn": function(label) {
			return label.indexOf("bytes") != -1;
		}
	},
	"garbageCollection": {
		"axisTitle": "Number of Garbace Collections",
		"unit": "",
		"scale": 1,
		"digits": 0,
		"isColumn": function(label) {
			return label == "number of GCs";
		}
	}
};

function showReplayData()
{
	var displayedColumn = $("#replayGraphSelection").val();

	$.plot($("#replayGraph"), getReplayGraphData(displayedColumn), {
		"grid": {
			"hoverable": true
		},
		"zoom": {
			"interactive": true
		},
		"pan": {
			"interactive": true
		},
		"legend": {
			"container": $("#replayGraphLegend")
		}
	});

	$("#replayGraph").bind("plothover", function (event, pos, item) {
		$("#tooltip").remove();
		if (!item)
			return;

		showTooltip(
			item.pageX,
			item.pageY,
			displayedColumn,
			item.series.label,
			item.datapoint[0],
			item.datapoint[1].toFixed(graphFormat[displayedColumn].digits));
	});

	$("#replayGraphDescription").html(
		"<p>X axis: Turn Number</p>" +
		"<p>Y axis: " + graphFormat[displayedColumn].axisTitle +
			(graphFormat[displayedColumn].unit ? " [" + graphFormat[displayedColumn].unit + "]" : "") + "</p>" +
		"<p>Drag to pan, mouse-wheel to zoom</p>"
	);
}

/**
 * Filter the affected columns and apply the scaling and rounding.
 */
function getReplayGraphData(displayedColumn)
{
	var replayGraphData = [];

	for (var i = 0; i < replayData.length; ++i)
	{
		var label = replayData[i].label;

		if (filteredColumns.indexOf(label) != -1 ||
		    (displayedColumn == "bytes") != graphFormat.bytes.isColumn(label) ||
		    (displayedColumn == "garbageCollection") != (graphFormat.garbageCollection.isColumn(label)))
			continue

		var data = [];
		for (var j = 0; j < replayData[i].data.length; ++j)
			data.push([
				replayData[i].data[j][0] * PROFILE_TURN_INTERVAL,
				replayData[i].data[j][1] * graphFormat[displayedColumn].scale
			]);

		replayGraphData.push({
			"label": label,
			"data": data
		});
	}

	return replayGraphData;
}

function showTooltip(x, y, displayedColumn, label, turn, value)
{
	$("body").append(
		$('<div id="tooltip">' + label + " at turn " + turn + ": " + value + " " + graphFormat[displayedColumn].unit + "</div>").css({
			"position": "absolute",
			"top": y + 5,
			"left": x + 5,
			"border": "1px solid #fdd",
			"padding": "2px",
			"background-color": "#fee",
			"opacity": 0.8
		}));
}

function loadReplayGraphData()
{
	$.ajax({
		"dataType": "json",
		"url": $("#filename").val(),
		"success": function(data) {
			$("#errorMsg").hide();
			$("#replayGraphContainer").show();
			replayData = data;
			showReplayData();
		},
		"error": function() {
			$("#replayGraphContainer").hide();
			$("#errorMsg").show();
		}
	});
}

$(loadReplayGraphData);
