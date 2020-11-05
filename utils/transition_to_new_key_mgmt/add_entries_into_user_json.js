// Adds new entries of master uuids into decrypted user json file


fs = require("fs");
execSync = require("child_process").execSync;

var working_dir = "intranet/";
var old_db_dir = working_dir + "files_all/";
var user_json_path = "fff.json";

//var files = fs.readdirSync(old_db_dir);
var files = fs.readdirSync("old_intranet/files_all/");

var user_json = JSON.parse(fs.readFileSync(user_json_path));
var owned = user_json.owned_data;

function add_entries()
{
	for (i in files) {

		var o = {};
		o.aes_iv = "";
		o.aes_key = "";
		o.copy_count = 3;
		o.is_master = true;
		o.owned = true;

		owned[files[i]] = o;
	}
	fs.writeFileSync(user_json_path, JSON.stringify(user_json), 'utf8');
}

add_entries();


