// Converts metax old format files to new format (without shared object)
// 
// Prerequisites synch-request and fs nodejs modules
//
fs = require("fs");
execSync = require("child_process").execSync;

// Please edit the variables below for you needs

// all working files should be located here
var working_dir = "intranet/";
// old format db folder, used to scan all shared objects
var old_db_dir = working_dir + "old_db/";
// stores all detected shared objects from old db
var share_objects = working_dir + "share_objects.txt";
// folder where all files taken from old db are saved for further save in a new
// format
var all_files_dir = working_dir + "files_all/";
// Metax db folder where files in new format are saved, make sure that config
// to the db of a new metax points to this folder
var new_db_dir = working_dir + "new_db/";


// gets the list of all shared objects in old db and saves the list into a file
function get_share_object_list() 
{
        var map = [];
        var files = fs.readdirSync(old_db_dir);

        for (i in files) {
                var file = files[i];
                var c;
                try {
                        c = fs.readFileSync(old_db_dir + file);
                        c = JSON.parse(c);
                        //console.log(file + " : valid json");
                        if (('aes_key' in c) && ('aes_iv' in c) && "master_uuid" in c) {
                                var o = {};
                                o.uuid = file;
                                if ("" == c.aes_key && "" == c.aes_iv) {
                                        o.enc = "0";
                                } else {
                                        o.enc = "1";
                                }
                                //console.log(file + " : is share object");
                                map.push(o);
                        } else {
                                //console.log(file + " : valid json but not master or encrypted");
                        }
                } catch (e) {
                }
        }
        fs.writeFileSync(share_objects, JSON.stringify(map), 'utf8');
}

// gets all files listed in share_objects.txt file from old db using old metax
// and saves the files into a folder with their uuuids as their filenames.
function get_files() 
{
        var map = JSON.parse(fs.readFileSync(share_objects));

        if (!fs.existsSync(all_files_dir)){
                fs.mkdirSync(all_files_dir);
        }
        for (i in map) {
                 var cmd = 'curl http://localhost:8001/db/get?id=' + map[i].uuid + 
                         ' > ' + all_files_dir +  map[i].uuid
                         + ' 2> /dev/null';
                 var v = execSync(cmd);
        }
}

// cycles on all uuids in share_objects.txt file and saves all the files with
// new metax into a new db format
function save_in_new_db()
{
        var map = JSON.parse(fs.readFileSync(share_objects));
        var new_to_old = {};

        for (i in map) {
                var cmd = 'curl --form fileupload=@' + all_files_dir +
                        map[i].uuid + ' http://localhost:8003/db/save/data?enc=' +
                        map[i].enc + ' 2> /dev/null';
                console.log("saving " + map[i].uuid);
                var v = execSync(cmd);
                v = JSON.parse(v);
                cmd = 'mv ' + new_db_dir + v.uuid + ' '
                        + new_db_dir + map[i].uuid;
                execSync(cmd);
                new_to_old[v.uuid] = map[i].uuid;
        }
        fs.writeFileSync(working_dir + 'new_to_old_map', JSON.stringify(new_to_old), 'utf8');
}


//get_share_object_list();
//get_files();
//save_in_new_db();

