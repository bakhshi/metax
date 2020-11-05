
// Removes entries listed below from share_objects.txt file

fs = require("fs");

var files = ["000dbf82-a51e-43d1-8deb-e89d58049c5c",
"00888283-cb5f-4147-b9c5-bf571721fc10",
"00a453ab-9956-4df6-a095-9ac24138bcd1",
"00a590a8-cdd2-4b5e-acf4-6f15ab7a938a",
"ffef1200-7f8f-4a32-8943-9b841f067de8"]

var map = JSON.parse(fs.readFileSync("intranet/share_objects.txt"));

var new_map = [];
for (i in map) {

	var found = files.find(function(element) {
		return element == map[i].uuid;
	});

	if (undefined == found) {
		new_map.push(map[i]);
	}
}
fs.writeFileSync("new_map", JSON.stringify(new_map), 'utf8');

