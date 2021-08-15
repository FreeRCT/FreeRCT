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

const MENU_BAR_BAR_HEIGHT = 50;
function readjustMenuBarY() {
	var totalMenuH = 408, logoMaxH = 256, menuSpacer = 298, bottomSpacer = 280, alwaysCollapse = false;
	if (window.matchMedia("(max-width: 1110px)").matches) {
		totalMenuH = 50;
		alwaysCollapse = true;
		logoMaxH = 50;
		menuSpacer = 70;
		bottomSpacer = 60;
		if (window.matchMedia("(max-width: 915px)").matches) {
			// TODO
		}
	} else if (window.matchMedia("(max-width: 1440px)").matches) {
		totalMenuH = 204;
		logoMaxH = 128;
		menuSpacer = 149;
		bottomSpacer = 140;
	}
	const menuBarMaxY = alwaysCollapse ? 0 : (totalMenuH - MENU_BAR_BAR_HEIGHT) / 2;

	const scroll = document.body.scrollTop / 1.2;
	const newBarY = Math.min(menuBarMaxY, Math.max(0, scroll));
	var logo = document.getElementById('menubar_logo');

	const newLogoH = logoMaxH - (alwaysCollapse ? 0 : ((logoMaxH - MENU_BAR_BAR_HEIGHT) * newBarY / menuBarMaxY));
	const newLogoHalfspace = (logoMaxH - newLogoH) / 2;
	// console.log("NOCOM: scroll @ " + scroll + ", barY " + newBarY + ", logo Max H " + logoMaxH + ", newLogoH " + newLogoH + ", newLogoHalfspace " + newLogoHalfspace);

	document.getElementById('menubar_ul').style.top = -newBarY;
	document.getElementById('menubar_spacer_menu').style.marginRight = menuSpacer;
	document.getElementById('menubar_spacer_bottom').style.marginBottom = bottomSpacer;
	logo.style.height = newLogoH;
	logo.style.marginLeft = newLogoHalfspace;
	logo.style.marginRight = newLogoHalfspace;
}
document.body.onscroll = readjustMenuBarY;
document.body.onresize = readjustMenuBarY;

function dropdownMouse(dd, inside) {
	dd.style = inside ? 'background-color: #003000' : '';
}

document.write('<link rel="icon" href="images/logo.png">');

document.write('<a class="pictorial_link" href=index.html>');
	document.write('<img id="menubar_logo" src="images/logo.png" height=auto width=auto></img>');
document.write('</a>');

document.write('<ul id="menubar_ul">');
	document.write('<link rel="stylesheet" href="menubar.css">');
	document.write('<p id="menubar_spacer_menu"></p>');

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
document.write('<p id="menubar_spacer_bottom"></p>');

document.write('<script>readjustMenuBarY();</script>');  // Need to call this last
