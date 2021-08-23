const ALL_PAGES = [
	{link: 'index'      , label: 'FreeRCT Home'},
	{link: 'download'   , label: 'Get It!'     },
	{link: 'manual'     , label: 'Manual'      },
	{link: 'screenshots', label: 'Screenshots' },
	{link: 'news'       , label: 'News Archive'},
	{link: 'contribute' , label: 'Contribute', dropdown: [
		{link: 'https://github.com/FreeRCT/FreeRCT'       , unique_id: 'github', absolute: true, label: 'Git Repository', newTab: true},
		{link: 'https://github.com/FreeRCT/FreeRCT/issues', unique_id: 'issues', absolute: true, label: 'Issue Tracker' , newTab: true},
	]},
];

const MENU_BAR_BAR_HEIGHT = 50;
const DESIRED_PADDING_BELOW_MENU_BAR = MENU_BAR_BAR_HEIGHT + 8;

var _all_links_created = {menubar: []};

function create_linkified_header(tag, doc, slug, text) {
	document.write('<' + tag + ' id="' + slug + '" style="padding-top:' + DESIRED_PADDING_BELOW_MENU_BAR + 'px">');
		document.write('<a href="' + doc + '.html#' + slug + '" class="linkified_header">');
		document.write(text);
	document.write('</a></' + tag + '>');

	register_linkified_header(tag, doc, slug, text);
}

function register_linkified_header(tag, doc, slug, text) {
	if (!_all_links_created[doc]) _all_links_created[doc] = [];
	_all_links_created[doc].push({tag: tag, slug: slug, text: text});
}

function makeHref(id, tooltiptype) {
	const unique_id = (id.unique_id ? id.unique_id : id.link);
	const unique_id_name = 'menubar_entry_' + unique_id;
	_all_links_created['menubar'].push({id: unique_id_name, link: unique_id});

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

function create_toc(doc) {
	document.write('<div class="toc"><h1><a href="' + doc + '.html#" class="linkified_header">ToC</a></h1><ul>');
		_all_links_created[doc].forEach((section) => {
			document.write('<li style="margin-left:' + (section.tag.substring(1) * 20 - 60) + 'px"><a href="' + doc + '.html#' + section.slug + '" class="linkified_header">');
				document.write(section.text);
			document.write('</a></li>');
		});
	document.write('</ul></div>');
}

function readjustMenuBarY() {
	var ul = document.getElementById('menubar_ul');
	var logo = document.getElementById('menubar_logo');
	var canvas = document.getElementById('menubar_top_canvas');
	var toc = document.getElementsByClassName("toc");
	var footer = document.getElementById('menubar_footer');

	var totalMenuH            =   510;
	var logoMaxH              =   320;
	var menuSpacer            =   372.5;
	var bottomSpacer          =   350;
	var fontSize              =    16;
	var alwaysCollapse        = false;
	var replaceTextWithImages = false;
	// Keep the `max-width` constants in sync with the constants in menubar.css!!!
	if (window.matchMedia("(max-width: 1150px)").matches) {
		totalMenuH     =   50;
		logoMaxH       =   50;
		menuSpacer     =   70;
		bottomSpacer   =   60;
		fontSize       =   14;
		alwaysCollapse = true;
		if (window.matchMedia("(max-width: 990px)").matches) {
			replaceTextWithImages = true;
		}
	} else if (window.matchMedia("(max-width: 1480px)").matches) {
		totalMenuH   = 306;
		logoMaxH     = 192;
		menuSpacer   = 223.5;
		bottomSpacer = 210;
		fontSize     =  15;
	}

	const menuBarMaxY = alwaysCollapse ? 0 : (totalMenuH - MENU_BAR_BAR_HEIGHT) / 2;

	const scroll = document.body.scrollTop / 1.2;
	const newBarY = Math.min(menuBarMaxY, Math.max(0, scroll));

	const newLogoH = logoMaxH - (alwaysCollapse ? 0 : ((logoMaxH - MENU_BAR_BAR_HEIGHT) * newBarY / menuBarMaxY));
	const newLogoHalfspace = (logoMaxH - newLogoH) / 2;

	canvas.style.height = menuBarMaxY - newBarY;

	const topPos = (totalMenuH - MENU_BAR_BAR_HEIGHT) / 2 - newBarY;
	ul.style.top = topPos;
	for (i = 0; i < toc.length; i++) {
		toc[i].style.top = topPos + MENU_BAR_BAR_HEIGHT;
	}

	document.getElementById('menubar_spacer_menu').style.marginRight = menuSpacer;
	document.getElementById('menubar_spacer_bottom').style.marginBottom = bottomSpacer;
	logo.style.height = newLogoH;
	logo.style.marginLeft = newLogoHalfspace;
	logo.style.marginRight = newLogoHalfspace;
	_all_links_created['menubar'].forEach((id) => {
		var element = document.getElementById(id.id);
		element.style.backgroundImage = replaceTextWithImages ? 'url(images/menu/' + id.link + '.png)' : 'none';
		element.style.color = replaceTextWithImages ? 'transparent' : 'var(--text-light)';
		element.style.fontSize = replaceTextWithImages ? '0' : (fontSize + 'px');
		element.style.minHeight = replaceTextWithImages ? '50px' : '0';
	});

	if (footer) {
		document.getElementById('footer_spacer').style.marginBottom = footer.clientHeight + 30;
		footer.style.opacity = Math.max(0.1, Math.min(1.0,
				1.0 - (document.body.scrollHeight - (window.innerHeight + window.pageYOffset)) / (footer.clientHeight * 2.0)));
	}
}

document.body.onscroll = readjustMenuBarY;
document.body.onresize = readjustMenuBarY;
document.body.onload   = readjustMenuBarY;

function dropdownMouse(dd, inside) {
	dd.style = inside ? 'background-color: var(--green-dark)' : '';
}

function copyrightfooter() {
	document.write('<p id="footer_spacer"></p><footer id="menubar_footer">© 2021 by the FreeRCT Development Team</footer>');
	document.write('<script>readjustMenuBarY();</script>');  // Need to call this last
}

document.write('<link rel="icon" href="images/logo.png">');
document.write('<div id="menubar_top_canvas"></div>');

document.write('<a class="pictorial_link" href=#top>');
	document.write('<img id="menubar_logo" src="images/logo.png" height=auto width=auto></img>');
document.write('</a>');

document.write('<ul id="menubar_ul">');
	document.write('<link rel="stylesheet" href="menubar.css">');
	document.write('<p id="menubar_spacer_menu"></p>');

	ALL_PAGES.forEach((id) => {
		if (id.dropdown == null) {
			document.write('<li class="menubar_li">');
			document.write(makeHref(id, 'tooltip_bottom'));
		} else {
			document.write('<li class="menubar_li menubar_dropdown" onmouseover="dropdownMouse(this, true)" onmouseout="dropdownMouse(this, false)">');
			document.write(makeHref(id, 'tooltip_corner'));
			document.write('<div class="menubar_dropdown_content">');
				id.dropdown.forEach((entry) => {
					document.write(makeHref(entry, 'tooltip_left'));
				});
			document.write('</div>');
		}
		document.write('</li>');
	});
document.write('</ul>');
document.write('<p id="menubar_spacer_bottom"></p>');
document.write('<script>readjustMenuBarY();</script>');  // Need to call this last
