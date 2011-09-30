/**
 * options, currentMonth, currentYear are global variable defined in index.html
 */

function showTooltip(x, y, contents) {
    $('<div id="tooltip">' + contents + '</div>').css( {
	position: 'absolute',
	display: 'none',
	top: y + 5,
	left: x + 5,
	border: '1px solid #fdd',
	padding: '2px',
	'background-color': '#fee',
	opacity: 0.80
    }).appendTo("body").fadeIn(200);
}

function getData (currentKeyword, data, func) {
    var dataUrlMarcel = "json/" + currentYear + "/" + currentMonth + "/marcel-" + currentKeyword.toLowerCase() + "-" + currentMonth + currentYear + ".json";
    var dataUrlNPTL = "json/" + currentYear + "/" + currentMonth + "/nptl-" + currentKeyword.toLowerCase() + "-" + currentMonth + currentYear + ".json";

    data = [];

    $.ajax({
        url: dataUrlMarcel,
        method: 'GET',
        dataType: 'json',
        success: func
    });

    $.ajax({
        url: dataUrlNPTL,
        method: 'GET',
        dataType: 'json',
        success: func
    });
}

function initTestHolders(testName, testData, testDataOnReceive) {
    testName[0] = "ContestedBarrierWait";
    testName[1] = "ContestedCondBroadcast";
    testName[2] = "ContestedCondSignal";
    testName[3] = "ContestedMutexLock";
    testName[4] = "ContestedMutexTrylock";
    testName[5] = "ContestedRwlockRdlock";
    testName[6] = "ContestedRwlockTryrdlock";
    testName[7] = "ContestedRwlockTrywrlock";
    testName[8] = "ContestedRwlockWrlock";
    testName[9] = "ContestedSemTryWait";
    testName[10] = "ContestedSemWait";
    testName[11] = "ContestedSpinLock";
    testName[12] = "ContestedSpinTrylock";
    testName[13] = "UncontestedCreate";
    testName[14] = "UncontestedMutexLock";
    testName[15] = "UncontestedMutexTrylock";
    testName[16] = "UncontestedRwlockRdlock";
    testName[17] = "UncontestedRwlockTryrdlock";
    testName[18] = "UncontestedRwlockTrywrlock";
    testName[19] = "UncontestedRwlockWrlock";
    testName[20] = "UncontestedSemTrywait";
    testName[21] = "UncontestedSemWait";
    testName[22] = "UncontestedSpinLock";
    testName[23] = "UncontestedSpinTrylock";

    var iter;
    for (iter = 0; iter < testName.length; iter ++) {
        testData[iter] = [];
	testDataOnReceive[iter] = function(series) {
	    // need to find who is registered to this event !!!
	    var iter;
	    for (iter = 0; iter < testName.length; iter ++) {
		if (arguments.callee == testDataOnReceive[iter])
		    break;
	    }

	    // let's add it to our current data
	    testData[iter].push(series);

	    // and plot all we got
	    $.plot("#" + getPlaceHolderName(testName[iter]), testData[iter], options);
	}

	// prepare the document for the graph !!!
	addGraphArea(document.getElementById('testGraphs'), testName[iter])
    }
}

function addGraphArea(rootNode, testname)
{
    var chartNode;
    var graphNode;
    var titleNode;

    chartNode = document.createElement("div");
    chartNode.setAttribute("style", "margin-top: 40px;");
    
    graphNode = document.createElement("div");
    graphNode.setAttribute("id", getPlaceHolderName(testname));
    graphNode.setAttribute("style", "width: 800px; height: 400px;");

    titleNode = document.createElement("div");
    titleNode.innerHTML = testname;

    chartNode.appendChild(graphNode);
    chartNode.appendChild(titleNode);
    rootNode.appendChild(chartNode);
}

function getPlaceHolderName(testname)
{
    return "placeHolder" + testname;
}

function clearPlots(testName, testData) {
    var iter;

    for (iter = 0; iter < testName.length; iter ++) {
	testData[iter] = [];
	$.plot("#" + getPlaceHolderName(testName[iter]), testData[iter], options);
    }
}

function printChoice (testName, testData, testDataOnReceive) {
    var iter;
    var yearIndex = document.forms.dateSelect.year.options.selectedIndex;
    var monthIndex = document.forms.dateSelect.month.options.selectedIndex;
    
    currentYear = document.forms.dateSelect.year.options[yearIndex].text;
    currentMonth = document.forms.dateSelect.month.options[monthIndex].text;
    
    clearPlots(testName, testData);
    for (iter = 0; iter < testName.length; iter ++)
	getData(testName[iter], testData[iter], testDataOnReceive[iter]);
}

function toolTipSetUp (placeholder) {
    $(placeholder).bind(
	"plothover", 
	function (event, pos, item) {
	    $("#x").text(pos.x.toFixed(2));
	    $("#y").text(pos.y.toFixed(2));
	    
	    if (item) {
		if (previousPoint != item.datapoint) {
		    previousPoint = item.datapoint;
		    
		    $("#tooltip").remove();
		    var day = item.datapoint[0].toFixed(0),
		    perf = item.datapoint[1];
		    
		    showTooltip(item.pageX, item.pageY,
				item.series.label + ", le " + day + " " + currentMonth + " " + currentYear + ", " + perf + " op/s");
		}
	    } else {
		$("#tooltip").remove();
		previousPoint = null;            
	    }
	});
}
