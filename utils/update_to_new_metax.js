// Converts metax old format files to new format (with shared object)
// The script updates only public (not encrypted) files.
// 
// Prerequisites synch-request and fs nodejs modules
// Metax web service should be up and running on 7071 port
//
// Usage: nodejs old_to_new.js <old_db> <new_db>
// 	  <old_db> - db with old format metax files
// 	  <new_db> - folder where files with new format are placed. Note, this is
// 	  not the entire db, but only new and updated files.

fs = require("fs")
var request = require('sync-request');
var uuidV4 = require('uuid/v4');

// Old and new db folders must be supplied, otherise print usage and exit
if (4 > process.argv.length) {
    console.log("Usage:\tnodejs old_to_new.js <old_db> <new_db>");
    console.log("\t<old_db> - db with old format metax files");
    console.log("\t<new_db> - folder where files with new format are placed. Note, this is not the entire db, but only new and updated files.");
    process.exit(1);
}


var shared_json = {
    "aes_iv" : "",
    "aes_key" : "",
    "master_uuid" : ""
};

var new_master_json = {
    "pieces": "",
    "size": 0
};

var old_db_dir = process.argv[2];
var new_db_dir = process.argv[3];

var files = fs.readdirSync(old_db_dir);

var map = {};

for (i in files) {
    var file = files[i];
    var c;
    try {
        c = fs.readFileSync(old_db_dir + file);
        c = JSON.parse(c);
        console.log(file + " : valid json");
        if (('aes_key' in c) && ('aes_iv' in c) && (c.aes_key == "") && (c.aes_iv == "")) {
            var new_uuid = uuidV4();
            new_master_json.pieces = c.pieces;
            new_master_json.size = c.size;
            fs.writeFileSync(new_db_dir + new_uuid,
                    JSON.stringify(new_master_json), 'utf8');
            shared_json.master_uuid = new_uuid;
            fs.writeFileSync(new_db_dir + file,
                    JSON.stringify(shared_json), 'utf8');
            map[file] = new_uuid;
        } else {
            console.log(file + " : valid json but not master or encrypted");
        }
    } catch (e) {
    }
}

fs.writeFileSync("bmap", JSON.stringify(map), 'utf8');

