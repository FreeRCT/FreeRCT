const ALL_PAGES = [
	/* Syntax: document name (minus the .html); Label in the menu bar; Open in new tab; Dropdown entries in the same format (or ``null`` if it's not a dropdown). */
	['index'      , 'FreeRCT Home', false, null],
	['download'   , 'Download'    , false, null],
	['manual'     , 'Manual'      , false, null],
	['screenshots', 'Screenshots' , false, null],
	['development', 'Development' , false, [
		['https://github.com/FreeRCT/FreeRCT'       , 'Git Repository', true],
		['https://github.com/FreeRCT/FreeRCT/issues', 'Issue Tracker' , true],
	]]
];

function makeHref(id) {
	const isAbsolute = id[0].startsWith('http');
	var hrefTag = '<a href="' + id[0]
	if (!isAbsolute) hrefTag += '.html';
	hrefTag += '"';

	if (!isAbsolute &&
		(document.URL.search('/docs/' + id[0] + '.html') >= 0 ||
			document.URL.search('/FreeRCT/' + id[0] + '.html') >= 0 ||
			(id[0] == 'index' && document.URL.endsWith('FreeRCT/')))) {
		hrefTag += ' class="menubar_active"';
	}

	if (id[2]) {
		hrefTag += ' target="_blank"';
	}

	hrefTag += '>' + id[1] + '</a>';
	return hrefTag;
}

document.write('<link rel="icon" href="images/logo.png">');
document.write('<ul>');
	document.write('<link rel="stylesheet" href="menubar.css">');
	document.write('<li style="width:auto">');
		document.write('<a class="pictorial_link" href=index.html>');
			document.write('<img src="images/logo.png" height="256" width="auto"></img>');
		document.write('</a>');
	document.write('</li>');

	ALL_PAGES.forEach(function(id) {
		if (id[3] == null) {
			document.write('<li>');
			document.write(makeHref(id));
		} else {
			document.write('<li class="dropdown">');
			document.write(makeHref(id));
			document.write('<div class="dropdown-content">');
				id[3].forEach(function(entry) {
					document.write(makeHref(entry));
				});
			document.write('</div>');
		}
		document.write('</li>');
	});
document.write('</ul>');
