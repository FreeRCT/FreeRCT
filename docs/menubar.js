const ALL_PAGES = [
	{link: 'index'      , label: 'FreeRCT Home'},
	{link: 'news'       , label: 'News'        },
	{link: 'download'   , label: 'Download'    },
	{link: 'manual'     , label: 'Manual'      },
	{link: 'screenshots', label: 'Screenshots' },
	{link: 'development', label: 'Development', dropdown: [
		{link: 'https://github.com/FreeRCT/FreeRCT'       , absolute: true, label: 'Git Repository', newTab: true},
		{link: 'https://github.com/FreeRCT/FreeRCT/issues', absolute: true, label: 'Issue Tracker' , newTab: true},
	]},
];

function makeHref(id) {
	var hrefTag = '<a href="' + id.link;
	if (!id.absolute) hrefTag += '.html';
	hrefTag += '"';

	if (!id.absolute &&
		(document.URL.search('/docs/' + id.link + '.html') >= 0 ||
			document.URL.search('/FreeRCT/' + id.link + '.html') >= 0 ||
			(id.link == 'index' && document.URL.endsWith('FreeRCT/')))) {
		hrefTag += ' class="menubar_active"';
	}

	if (id.newTab) {
		hrefTag += ' target="_blank"';
	}

	hrefTag += '>' + id.label + '</a>';
	return hrefTag;
}

const MENU_BAR_MAX_HEIGHT = 256;
const MENU_BAR_BAR_HEIGHT = 50;
const MENU_BAR_MAX_Y = (MENU_BAR_MAX_HEIGHT - MENU_BAR_BAR_HEIGHT) / 2;
function readjustMenuBarY() {
	const scroll = document.body.scrollTop / 2;
	const newBarY = Math.min(MENU_BAR_MAX_Y, Math.max(0, scroll));
	const newLogoH = MENU_BAR_MAX_HEIGHT - newBarY * (MENU_BAR_MAX_HEIGHT - MENU_BAR_BAR_HEIGHT) / MENU_BAR_MAX_Y;
	const newLogoHalfspace = (MENU_BAR_MAX_HEIGHT - newLogoH) / 2;

	document.getElementById('menubar_ul').style.top = -newBarY;
	var logo = document.getElementById('menubar_logo');
	logo.height = newLogoH;
	logo.style = 'margin-left:' + newLogoHalfspace + ';margin-right:' + newLogoHalfspace; 
}
document.body.onscroll = readjustMenuBarY;

function dropdownMouse(dd, inside) {
	dd.style = inside ? 'background-color: #003000' : '';
}

document.write('<link rel="icon" href="images/logo.png">');

document.write('<a class="pictorial_link" href=index.html>');
	document.write('<img id="menubar_logo" src="images/logo.png" height="256" width="auto"></img>');
document.write('</a>');

document.write('<ul id="menubar_ul">');
	document.write('<link rel="stylesheet" href="menubar.css">');
	document.write('<p style="margin-right:298px"></p>');

	ALL_PAGES.forEach(function(id) {
		if (id.dropdown == null) {
			document.write('<li>');
			document.write(makeHref(id));
		} else {
			document.write('<li class="dropdown" onmouseover="dropdownMouse(this, true)" onmouseout="dropdownMouse(this, false)">');
			document.write(makeHref(id));
			document.write('<div class="dropdown-content">');
				id.dropdown.forEach(function(entry) {
					document.write(makeHref(entry));
				});
			document.write('</div>');
		}
		document.write('</li>');
	});
document.write('</ul>');
document.write('<script>readjustMenuBarY();</script>');
document.write('<p style="margin-bottom:280px"></p>');
