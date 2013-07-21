function sortDecreasingDate(a, b)
{
	return b.metadata.time - a.metadata.time;
}

function twoDigits(n)
{
	return n < 10 ? "0" + n : n;
}

function generateLabel(metadata)
{
	var t = new Date(metadata.time*1000);

	var date = t.getFullYear()+"-"+twoDigits(1+t.getMonth())+"-"+twoDigits(t.getDate());
	var time = twoDigits(t.getHours())+":"+twoDigits(t.getMinutes())+":"+twoDigits(t.getSeconds());
	return "["+date+" "+time+"] "+metadata.initAttributes.map+(metadata.description ? " - "+metadata.description : "");
}
