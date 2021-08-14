const ALL_PAGES = [
	/* Syntax: document name (minus the .html), Label in the menu bar */
	['index'      , 'FreeRCT Home'],
	['download'   , 'Download'    ],
	['manual'     , 'Manual'      ],
	['screenshots', 'Screenshots' ],
	['development', 'Development' ]
];

document.write('<ul> <link rel="stylesheet" href="menubar.css"> <li style="width:auto"><a class="pictorial_link" href=index.html><img src="images/logo.png" height="256" width="auto"></img></a></li>');
ALL_PAGES.forEach(function(id) {
	document.write('<li><a href="' + id[0] + '.html"');
	if (document.URL.search('/docs/' + id[0] + '.html') >= 0 ||
			document.URL.search('/FreeRCT/' + id[0] + '.html') >= 0 ||
			(id[0] == 'index' && document.URL.endsWith('FreeRCT/'))) {
		document.write(' class="menubar_active"');
	}
	document.write('>' + id[1] + '</a></li>');
});

document.write('</ul>');
