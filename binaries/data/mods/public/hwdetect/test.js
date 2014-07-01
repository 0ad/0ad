// Run in a standalone JS shell like
//   js -e 'var global={}' -f hwdetect.js -f test_data.js -f test.js > output.html
// where test_data.js is a giant file that's probably not publicly available yet
// (ask Philip if you want a copy), then look at output.html and make sure it's
// applying the hwdetected settings in the appropriate cases

print("<!DOCTYPE html>");
print("<meta charset=utf-8>");
print("<style>body { font: 8pt sans-serif; }</style>");
print("<table>");
print("<tr>");
print("<th>OS");
print("<th>GL_RENDERER");
print("<th>Output");
print("<th>Warnings");

hwdetectTestData.sort(function(a, b) {
	if (a.GL_RENDERER < b.GL_RENDERER)
		return -1;
	if (b.GL_RENDERER < a.GL_RENDERER)
		return +1;
	return 0;
});

for each (var settings in hwdetectTestData)
{
	var output = RunDetection(settings);

	var os = (settings.os_linux ? "linux" : settings.os_macosx ? "macosx" : settings.os_win ? "win" : "???");

	var disabled = [];
	for each (var d in ["disable_audio", "disable_s3tc", "disable_shadows", "disable_shadowpcf", "disable_allwater", "disable_fancywater", "override_renderpath"])
		if (output[d] !== undefined)
			disabled.push(d+"="+output[d])

	print("<tr>");
	print("<td>" + os);
	print("<td>" + settings.GL_RENDERER);
	print("<td>" + disabled.join(" "));
	print("<td>" + output.warnings.concat(output.dialog_warnings).join("\n"));
}

print("</table>");
