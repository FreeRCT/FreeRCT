var slideIndex = 0;

var timeoutVar = null;
showSlidesAuto();

// Next/previous controls
function plusSlides(n) {
	showSlides(slideIndex += n);
}

// Thumbnail image controls
function currentSlide(n) {
	showSlides(slideIndex = n);
}

function showSlides(n) {
	var i;
	var images = document.getElementsByClassName("slideshow_image");
	var texts = document.getElementsByClassName("slideshow_text");
	var dots = document.getElementsByClassName("slideshow_dot");
	if (n > images.length) slideIndex = 1;
	if (n < 1) slideIndex = images.length;

	for (i = 0; i < images.length; i++) {
		images[i].style.opacity = (i + 1 == slideIndex) ? 1 : 0;
		texts[i].style.opacity = (i + 1 == slideIndex) ? 1 : 0;
	}
	for (i = 0; i < dots.length; i++) dots[i].className = dots[i].className.replace(" slideshow_dot_active", "");
	dots[slideIndex-1].className += " slideshow_dot_active";

	if (timeoutVar) clearTimeout(timeoutVar);
	timeoutVar = [setTimeout(showSlidesAuto, 5000)];
}

function showSlidesAuto() {
	plusSlides(1);
}
