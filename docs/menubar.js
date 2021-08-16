const ALL_PAGES = [
	{link: 'index'      , label: 'FreeRCT Home'},
	{link: 'news'       , label: 'News'        },
	{link: 'download'   , label: 'Download'    },
	{link: 'manual'     , label: 'Manual'      },
	{link: 'screenshots', label: 'Screenshots' },
	{link: 'development', label: 'Development', dropdown: [
		{link: 'https://github.com/FreeRCT/FreeRCT'       , unique_id: 'github', absolute: true, label: 'Git Repository', newTab: true},
		{link: 'https://github.com/FreeRCT/FreeRCT/issues', unique_id: 'issues', absolute: true, label: 'Issue Tracker' , newTab: true},
	]},
];

const MENU_BAR_BAR_HEIGHT = 50;
const DESIRED_PADDING_BELOW_MENU_BAR = MENU_BAR_BAR_HEIGHT + 8;

var _all_links_created = [];
function makeHref(id, tooltiptype) {
	const unique_id = (id.unique_id ? id.unique_id : id.link);
	const unique_id_name = 'menubar_entry_' + unique_id;
	_all_links_created.push({id: unique_id_name, link: unique_id});

	var hrefTag = '<a id="' + unique_id_name + '" href="' + id.link;
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

	hrefTag += '>' + id.label + '<span class="' + tooltiptype + '">' + id.label + '</span></a>';
	return hrefTag;
}

function readjustMenuBarY() {
	var ul = document.getElementById('menubar_ul');
	var logo = document.getElementById('menubar_logo');
	var canvas = document.getElementById('menubar_top_canvas');

	const widthType = getComputedStyle(ul).getPropertyValue('--menubar-width-type');
	var
		totalMenuH            =   408,
		logoMaxH              =   256,
		menuSpacer            =   298,
		bottomSpacer          =   280,
		fontSize              =    16,
		alwaysCollapse        = false,
		replaceTextWithImages = false;
	if (widthType == '"normal"') {
		totalMenuH   = 204;
		logoMaxH     = 128;
		menuSpacer   = 149;
		bottomSpacer = 140;
		fontSize     =  14;
	} else if (widthType != '"wide"') {
		totalMenuH     =   50;
		logoMaxH       =   50;
		menuSpacer     =   70;
		bottomSpacer   =   60;
		fontSize       =   12;
		alwaysCollapse = true;
		replaceTextWithImages = (widthType == '"mobile"');
	}

	const menuBarMaxY = alwaysCollapse ? 0 : (totalMenuH - MENU_BAR_BAR_HEIGHT) / 2;

	const scroll = document.body.scrollTop / 1.2;
	const newBarY = Math.min(menuBarMaxY, Math.max(0, scroll));

	const newLogoH = logoMaxH - (alwaysCollapse ? 0 : ((logoMaxH - MENU_BAR_BAR_HEIGHT) * newBarY / menuBarMaxY));
	const newLogoHalfspace = (logoMaxH - newLogoH) / 2;

	canvas.style.height = menuBarMaxY - newBarY;

	ul.style.top = (totalMenuH - MENU_BAR_BAR_HEIGHT) / 2 - newBarY;
	document.getElementById('menubar_spacer_menu').style.marginRight = menuSpacer;
	document.getElementById('menubar_spacer_bottom').style.marginBottom = bottomSpacer;
	logo.style.height = newLogoH;
	logo.style.marginLeft = newLogoHalfspace;
	logo.style.marginRight = newLogoHalfspace;
	_all_links_created.forEach(function(id) {
		var element = document.getElementById(id.id);
		element.style.backgroundImage = replaceTextWithImages ? 'url(images/menu/' + id.link + '.png)' : 'none';
		element.style.color = replaceTextWithImages ? 'transparent' : 'var(--text-light)';
		element.style.fontSize = replaceTextWithImages ? '0' : (fontSize + 'px');
		element.style.minHeight = replaceTextWithImages ? '50px' : '0';
	});
}

document.body.onscroll = readjustMenuBarY;
document.body.onresize = readjustMenuBarY;
document.body.onload = readjustMenuBarY;

function dropdownMouse(dd, inside) {
	dd.style = inside ? 'background-color: var(--green-dark)' : '';
}

document.write('<link rel="icon" href="images/logo.png">');
document.write('<div id="menubar_top_canvas"></div>');

document.write('<a class="pictorial_link" href=#top>');
	document.write('<img id="menubar_logo" src="images/logo.png" height=auto width=auto></img>');
document.write('</a>');

document.write('<ul id="menubar_ul">');
	document.write('<link rel="stylesheet" href="menubar.css">');
	document.write('<p id="menubar_spacer_menu"></p>');

	ALL_PAGES.forEach(function(id) {
		if (id.dropdown == null) {
			document.write('<li class="menubar_li">');
			document.write(makeHref(id, 'tooltip_bottom'));
		} else {
			document.write('<li class="menubar_li menubar_dropdown" onmouseover="dropdownMouse(this, true)" onmouseout="dropdownMouse(this, false)">');
			document.write(makeHref(id, 'tooltip_corner'));
			document.write('<div class="menubar_dropdown_content">');
				id.dropdown.forEach(function(entry) {
					document.write(makeHref(entry, 'tooltip_left'));
				});
			document.write('</div>');
		}
		document.write('</li>');
	});
document.write('</ul>');
document.write('<p id="menubar_spacer_bottom"></p>');
document.write('<script>readjustMenuBarY();</script>');  // Need to call this last
