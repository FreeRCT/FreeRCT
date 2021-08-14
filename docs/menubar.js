const ALL_PAGES = [
	/* Syntax: document name (minus the .html), Label in the menu bar */
	['index'      , 'FreeRCT Home'],
	['download'   , 'Download'    ],
	['manual'     , 'Manual'      ],
	['screenshots', 'Screenshots' ],
	['development', 'Development' ]
];

document.write('<link rel="icon" href="images/logo.png">');
document.write('<ul>');
	document.write('<link rel="stylesheet" href="menubar.css">');
	document.write('<li style="width:auto">');
		document.write('<a class="pictorial_link" href=index.html>');
			document.write('<img src="images/logo.png" height="256" width="auto"></img>');
		document.write('</a>');
	document.write('</li>');

	ALL_PAGES.forEach(function(id) {
		document.write('<li>');
			document.write('<a href="' + id[0] + '.html"');
			if (document.URL.search('/docs/' + id[0] + '.html') >= 0 ||
					document.URL.search('/FreeRCT/' + id[0] + '.html') >= 0 ||
					(id[0] == 'index' && document.URL.endsWith('FreeRCT/'))) {
				document.write(' class="menubar_active"');
			}
			document.write('>' + id[1] + '</a>');
		document.write('</li>');
	});
document.write('</ul>');
