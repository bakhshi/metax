// Updates user json based on a map. Chnages the existing uuid by the new one
// provided in the map. The map should contain the list of key-value pairs,
// where key is the current value in the user json and value is the new one by
// which the key should be replaced.
// Note, the appropriate map is being crated by convert_db.js script in
// save_in_new_db function

fs = require("fs")

var user_json_path = "k.json";
var working_dir = "./";

var user_json = JSON.parse(fs.readFileSync(user_json_path));

var map = JSON.parse(fs.readFileSync("nto.json"));

owned = user_json.owned_data;


Object.keys(map).forEach(function(element, key, _array) {
	console.log(element + " : " + map[element]);

	user_json.owned_data[map[element]] = user_json.owned_data[element];
	delete user_json.owned_data[element];
});

fs.writeFileSync(user_json_path, JSON.stringify(user_json), 'utf8');

