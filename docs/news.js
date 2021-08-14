const NEWS = [
	{
		date: new Date(2021, 8, 14, 10, 0, 0, 0),
		title: 'New FreeRCT Homepage',
		body: "<p>Development on the new FreeRCT website has started today. You're looking at the result right now.</p><p>The site is still heavily under development, please be patient until we're done.</p>",
	},
	{
		date: new Date(2021, 8, 14, 14, 0, 0, 0),
		title: 'FreeRCT 0.1 Coming Soon',
		body: "<p>The FreeRCT project is making great progress. Expect the release of the <strong>first release 0.1</strong> sometime soon!</p>",
	},
].sort(function(a, b) { return a.date == b.date ? 0 : a.date < b.date ? 1 : -1; });

function printNews(n) {
	document.write('<div class="news">');
		document.write('<h3>');
			document.write(n.title);
		document.write('</h3>');
		document.write(n.body);
		document.write('<p class="news_timestamp">~ ');
			document.write(n.date.toString());
		document.write('</p>');
	document.write('</div>');
}

function printLatestNews(howMany) {
	if (isNaN(howMany) || howMany > NEWS.length) howMany = NEWS.length;
	for (var i = 0; i < howMany; i++) printNews(NEWS[i]);
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
	printLatestNews()
}
