const ALL_SLIDES = [
	{image: '20150609-freerct', caption: 'FreeRCT …'},
	{image: '20121209-freerct', caption: '… aims to be a free and open source game …'},
	{image: 'closeup', caption: '… which captures the look, …'},
	{image: 'make_a_path', caption: '… feel, …'},
	{image: 'find_the_differences', caption: '… and gameplay …'},
	{image: 'path_finding2', caption: '… of the popular games …'},
	{image: 'crowded', caption: '… RollerCoaster Tycoon 1 and 2.'},
	{image: 'queuepaths', caption: 'The game is still in an early alpha state, …'},
	{image: 'ice_creams_recoloured', caption: '… but it is already playable …'},
	{image: 'weather', caption: '… and offers a variety of features.'},
];

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
			ALL_SLIDES.forEach(function(slide) {
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
