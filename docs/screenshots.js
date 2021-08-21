const ALL_SCREENSHOT_SECTIONS = [
	{slug: '0_1', label: 'FreeRCT 0.1'},
	{slug: 'test', label: 'Test Images'},  // TODO delete
];
const ALL_IMAGES = [  // TODO need more and better images
	{image: 'OLD/20150609-freerct'     , slideshow: true, section: '0_1' , caption: 'FreeRCT …'},
	{image: 'OLD/20121209-freerct'     , slideshow: true, section: '0_1' , caption: '… aims to be a free and open source game …'},
	{image: 'OLD/closeup'              , slideshow: true, section: 'test', caption: '… which captures the look, …'},
	{image: 'OLD/make_a_path'          , slideshow: true, section: 'test', caption: '… feel, …'},
	{image: 'OLD/find_the_differences' , slideshow: true, section: 'test', caption: '… and gameplay …'},
	{image: 'OLD/path_finding2'        , slideshow: true, section: 'test', caption: '… of the popular games …'},
	{image: 'OLD/crowded'              , slideshow: true, section: '0_1' , caption: '… RollerCoaster Tycoon 1 and 2.'},
	{image: 'OLD/queuepaths'           , slideshow: true, section: 'test', caption: 'The game is still in an early alpha state, …'},
	{image: 'OLD/ice_creams_recoloured', slideshow: true, section: '0_1' , caption: '… but it is already playable …'},
	{image: 'OLD/weather'              , slideshow: true, section: '0_1' , caption: '… and offers a variety of features.'},
];

// Gallery code below.

var currentGalleryPopup = null;

function createScreenshotGallery() {
	ALL_SCREENSHOT_SECTIONS.forEach((section) => {
		document.write('<h2 id="' + section.slug + '" style="padding-top:' + DESIRED_PADDING_BELOW_MENU_BAR + 'px">');
			document.write('<a href="screenshots.html#' + section.slug + '" class="linkified_header">');
				document.write(section.label);
		document.write('</a></h2><div class="screenshot_gallery">');
		var all_images_in_section = [];
		ALL_IMAGES.forEach((img) => { if (img.section == section.slug) all_images_in_section.push(img); });

		for (var i = 0; i < all_images_in_section.length; i++) {
			const img = all_images_in_section[i];
			document.write('<img class="screenshot_gallery_image" loading=lazy src="images/' + img.image +
			               '.png" height="auto" width="auto" onclick="screenshot_image_clicked(event.currentTarget)"></img>');
			document.write('<div class="screenshot_gallery_popup_outer_wrapper"><div class="screenshot_gallery_popup_inner_wrapper">');
				document.write('<div class="screenshot_gallery_popup_prev" onclick="gallery_next(-1)">&#10094;</div>');
				document.write('<div class="screenshot_gallery_popup_next" onclick="gallery_next(+1)">&#10095;</div>');
				document.write('<div class="screenshot_gallery_popup_close" onclick="hide_screenshot_images()">&#128937;</div>');
				document.write('<img class="screenshot_gallery_popup_image" src="images/' + img.image +
					           '.png" height="auto" width="auto" onclick="event.stopPropagation()"></img>');
			document.write('</div></div>');
		}
		document.write('</div>');
	});
	hide_screenshot_images();
}

function screenshot_image_clicked(element) {
	hide_screenshot_images();

	while (element.className != 'screenshot_gallery_image' && element.className != 'screenshot_gallery_popup_outer_wrapper') {
		element = element.parentNode;
		if (element == null) return;
	}
	if (element.className == 'screenshot_gallery_image') element = element.nextSibling;
	element.style.display = 'initial';
	currentGalleryPopup = element;

	event.stopPropagation();
}

function gallery_next(delta) {
	var element = currentGalleryPopup;
	while (element.className != 'screenshot_gallery') {
		element = element.parentNode;
		if (!element) return;
	}

	var all = element.getElementsByClassName(currentGalleryPopup.className);
	for (var i = 0; i < all.length; i++) {
		if (all[i] == currentGalleryPopup) {
			i += delta;
			while (i < 0) i += all.length;
			i %= all.length;
			screenshot_image_clicked(all[i]);
			return;
		}
	}
}

function gallery_key_event() {
	if (currentGalleryPopup != null) {
		if (event.key == 'Escape') {
			hide_screenshot_images();
		} else if (event.key == 'ArrowLeft') {
			gallery_next(-1);
		} else if (event.key == 'ArrowRight') {
			gallery_next(+1);
		}
	}
}

function hide_screenshot_images() {
	var items = document.getElementsByClassName("screenshot_gallery_popup_outer_wrapper");
	for (var i = 0; i < items.length; i++) {
		items[i].style.display = 'none';
	}
	currentGalleryPopup = null;
}
document.body.onclick = hide_screenshot_images;
document.body.onkeydown = gallery_key_event;

// Slideshow code below.

var ALL_SLIDES = [];
ALL_IMAGES.forEach((s) => { if (s.slideshow) ALL_SLIDES.push(s); });
var slideIndex = 0;
var timeoutVar = null;

function plusSlides(n) {
	showSlides(slideIndex += n);
}

function currentSlide(n) {
	showSlides(slideIndex = n);
}

function showSlides(n) {
	var i;
	var images = document.getElementsByClassName("slideshow_image");
	var texts = document.getElementsByClassName("slideshow_text");
	var dots = document.getElementsByClassName("slideshow_dot");
	if (n > ALL_SLIDES.length) slideIndex = 1;
	if (n < 1) slideIndex = ALL_SLIDES.length;

	for (i = 0; i < ALL_SLIDES.length; i++) {
		images[i].style.opacity = (i + 1 == slideIndex) ? 1 : 0;
		texts[i].style.opacity = (i + 1 == slideIndex) ? 1 : 0;
	}
	for (i = 0; i < ALL_SLIDES.length; i++) dots[i].className = dots[i].className.replace(" slideshow_dot_active", "");
	dots[slideIndex-1].className += " slideshow_dot_active";

	if (timeoutVar) clearTimeout(timeoutVar);
	timeoutVar = [setTimeout(showSlidesAuto, 5000)];
}

function showSlidesAuto() {
	plusSlides(1);
}

function createSlideshow() {
	document.write('<div class="slideshow_main">');
		document.write('<div class="slideshow_container">');
			ALL_SLIDES.forEach((slide) => {
				document.write('<div class="slideshow_slide');
				if (slide == ALL_SLIDES[0]) document.write(' slideshow_first_slide');
				document.write('"><img class="slideshow_image" src="images/' + slide.image + '.png"></img>');
				document.write('<div class="slideshow_text">' + slide.caption + '</div>');
				document.write('</div>');
			});
		document.write('<a class="slideshow_prev" onclick="plusSlides(-1)">&#10094;</a>');
		document.write('<a class="slideshow_next" onclick="plusSlides(1)">&#10095;</a>');
		document.write('</div><br><div style="text-align:center; margin-top:24px">');
			for (var i = 1; i <= ALL_SLIDES.length; i++) {
				document.write('<span class="slideshow_dot" onclick="currentSlide(' + i + ')"></span>');
			}
		document.write('</div>');
	document.write('</div>');

	showSlidesAuto();
}
