const NEWS = [
	{
		date: new Date(2021, 8, 14, 10, 0, 0, 0),
		slug: 'new_freerct_homepage',
		title: 'New FreeRCT Homepage',
		body: "<p>Development on the new FreeRCT website has started today. You're looking at the result right now.</p><p>The site is still heavily under development, please be patient until we're done.</p>",
	},
	{
		date: new Date(2021, 8, 14, 14, 0, 0, 0),
		slug: 'test',
		title: 'Test',
		body: "Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Aenean commodo ligula eget dolor. Aenean massa. Cum sociis natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Donec quam felis, ultricies nec, pellentesque eu, pretium quis, sem. Nulla consequat massa quis enim. Donec pede justo, fringilla vel, aliquet nec, vulputate eget, arcu. In enim justo, rhoncus ut, imperdiet a, venenatis vitae, justo. Nullam dictum felis eu pede mollis pretium. Integer tincidunt. Cras dapibus. Vivamus elementum semper nisi. Aenean vulputate eleifend tellus. Aenean leo ligula, porttitor eu, consequat vitae, eleifend ac, enim. Aliquam lorem ante, dapibus in, viverra quis, feugiat a, tellus.",
	},
	{
		date: new Date(2021, 8, 14, 22, 0, 0, 0),
		slug: 'freerct_0_1_coming_soon',
		title: 'FreeRCT 0.1 Coming Soon',
		body: "<p>The FreeRCT project is making great progress. Expect the release of the <strong>first release 0.1</strong> sometime soon!</p>",
	},
].sort(function(a, b) { return a.date == b.date ? 0 : a.date < b.date ? 1 : -1; });

function printNews(n, margin) {
	document.write('<p id="' + n.slug + '" style="margin-bottom:' + margin + 'px"></p><div class="news">');
		document.write('<h3><a href="news.html#' + n.slug + '" class="linkified_header">');
			document.write(n.title);
		document.write('</a></h3>');
		document.write(n.body);
		document.write('<p class="news_timestamp">~ ');
			document.write(n.date.toString());
		document.write('</p>');
	document.write('</div>');
}

function printLatestNews(howMany, margin) {
	if (isNaN(howMany) || howMany > NEWS.length) howMany = NEWS.length;
	for (var i = 0; i < howMany; i++) printNews(NEWS[i], margin);
}

function printAllNews() {
	document.write('<h2 class="news_count">');
		document.write(NEWS.length);
		if (NEWS.length == 1) {
			document.write(' news item');
		} else {
			document.write(' news items');
		}
	document.write('</h2>');
	printLatestNews(NaN, MENU_BAR_BAR_HEIGHT);
}
