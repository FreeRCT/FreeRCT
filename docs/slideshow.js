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
	var slides = document.getElementsByClassName("mySlides");
	var dots = document.getElementsByClassName("slideshow_dot");
	if (n > slides.length) slideIndex = 1;
	if (n < 1) slideIndex = slides.length;

	for (i = 0; i < slides.length; i++) slides[i].style.display = "none";
	for (i = 0; i < dots.length; i++) dots[i].className = dots[i].className.replace(" slideshow_dot_active", "");
	slides[slideIndex-1].style.display = "block";
	dots[slideIndex-1].className += " slideshow_dot_active";

	if (timeoutVar != null) clearTimeout(timeoutVar);
	timeoutVar = setTimeout(showSlidesAuto, 5000);
}

function showSlidesAuto() {
	plusSlides(1);
} 
