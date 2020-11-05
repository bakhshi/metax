// The script creates actions from commited actions list into Metax storage.
// Prerequisites:
//   NodeJS (^4)
//   npm modules: jsonlint, sync-request, fs, child_process
//
// All configs can be changed from the global variables list.
// Usage: nodejs migrate.js [-save/-get]
// -save: Saves from actions main file separate antions into metax,
// -get: Gets actions from metax and creates linted json file for all actions.


//Global variables

var request = require("sync-request");
var fs = require("fs");
var exec = require("child_process").exec;
var jslint = require("jsonlint");

var currPath = process.cwd();
var actionsFile = currPath + "/actions.json";
var actionsFileNew = currPath + "/actions_row.json";
var actionsResources = currPath + "/resources/";
var actionsStorage = currPath + "/res/";
var host = "http://localhost:7071/";
var uriSaveNode = "db/save/node?enc=0";
var uriSavePath = "db/save/path?enc=0";
var uriGet = "db/get?id=";

//Actions list is - script, include_title, include_txt, edit_title, add_node, edit_img,
// hide_node, tags, edit_txt, edit_place, edit_expand.
var data = {
          "00000000-0000-0000-0000-000000000001" : ""
        , "00000000-0000-0000-0000-000000000002" : ""
        , "00000000-0000-0000-0000-000000000003" : ""
        , "00000000-0000-0000-0000-000000000004" : ""
        , "00000000-0000-0000-0000-000000000005" : ""
        , "00000000-0000-0000-0000-000000000006" : ""
        , "00000000-0000-0000-0000-000000000007" : ""
        , "00000000-0000-0000-0000-000000000008" : ""
        , "00000000-0000-0000-0000-000000000009" : ""
        , "00000000-0000-0000-0000-000000000010" : ""
        , "00000000-0000-0000-0000-000000000011" : ""
        , "00000000-0000-0000-0000-000000000012" : ""
        , "00000000-0000-0000-0000-000000000013" : ""
        , "00000000-0000-0000-0000-000000000014" : ""
};

var lint_actions = function() {
    var cmd = "cat " + actionsFileNew + " | " + "jsonlint > " + actionsFile;
    var l = exec(cmd, function(err, stdout, stderr) {
        if (err) {
            console.log("Error in linting by jsonlint: " + err);
        }
        console.log("Successfully linted " + stdout);
        console.log("Saved actions into: " + actionsFile + " as linted json.");
    });
};

var save_file = function(name, file) {
    if (! fs.existsSync(actionsResources)) {
         fs.mkdirSync(actionsResources)
    }
    fs.writeFileSync(actionsResources + name, file);
};

var get_actions = function() {
    console.log("--- Start of get actions ---");
    var old_actions = (fs.existsSync(actionsFile)) ?
                       JSON.parse(fs.readFileSync(actionsFile, "utf8")) : {};
    var req = host + uriGet;
    var actions_count = 0;
    if (old_actions.length > data.length) {
        data = old_actions;
    }
    for (i in data) {
        console.log("-> Getting " + i);
        if (fs.existsSync(actionsStorage + i)) {
            var url = req + i;
            var res = request("get", url);
            data[i] = JSON.parse(res.body);
            var img = data[i].img.value;
            if (img) {
                url = req + img;
                res = request("get", url);
                save_file(img, res.body);
            }
            if (old_actions[i]) {
                console.log("-> Updated " + i + " in " + actionsFileNew);
            } else {
                console.log("-> Added " + i + " in " + actionsFileNew);
            }
            old_actions[i] = data[i];
            actions_count++;
        } else {
            console.log("-> Does not exist " + i + ", returning...");
            return;
        }
    }
    console.log("Actions count is: " + actions_count);
    fs.writeFileSync(actionsFileNew, JSON.stringify(old_actions));
    console.log("Saved actions into new file: " + actionsFileNew + " as row.");
    lint_actions();
    console.log("--- End of get actions ---");
};

var copy_file = function(old_file, new_file, callback) {
    var cb = false;
    var rd = fs.createReadStream(old_file);
    rd.on("error", function(err) {
        done(err);
    });
    var wr = fs.createWriteStream(new_file);
    wr.on("error", function(err) {
        done(err);
    });
    wr.on("close", function(ex) {
        done();
    });
    rd.pipe(wr);
    function done(err) {
        if (!cb) {
            if (callback) callback(err);
            cb = true;
        }
    }
};

// Copies files for specified map
var copy_actions = function(uuids_map) {
    console.log("--- Start of copy actions ---");
    for (i in uuids_map) {
        if (fs.existsSync(i)) {
            if (fs.existsSync(uuids_map[i])) {
                console.log("-> Updating action " + i + "\n\t to " + uuids_map[i]);
            } else {
                console.log("-> Renamed action " + i + "\n\t to " + uuids_map[i]);
            }
            // Renames new generated uuid to the old action specific uuid
            copy_file(i, uuids_map[i]);
        }
    }
    console.log("--- End of copy actions ---");
};

var save_resource = function(img) {
    var path = actionsResources + img
    if (fs.existsSync(path)) {
        var url = host + uriSavePath;
        var res = request("post", url, { body: path });
        var uuid = JSON.parse(res.body)["uuid"];
        console.log("-> Saved resource " + path + "\n\t by uuid " + uuid);
        return uuid;
    }
};

var save_actions = function() {
    console.log("--- Start of save actions ---");
    var actions = JSON.parse(fs.readFileSync(actionsFile, "utf8"));
    var req = host + uriSaveNode;
    var actions_count = 0;
    //Map of new uuid and old uuid
    var uuids_map = {};
    for (i in actions) {
        var url = req;
        console.log("-> Saving " + url);
        var img = actions[i].img.value;
        if (img) {
            var n_img = actionsStorage + "/" + save_resource(img);
            var o_img = actionsResources + "/" + img;
            uuids_map[n_img] = o_img;
        }
        var data = JSON.stringify(actions[i]);
        var res = request("post", url, { body: data });
        var uuid = JSON.parse(res.body)["uuid"];
        actions_count++;
        var o = actionsStorage + "/" + i;
        var n = actionsStorage + "/" + uuid;
        uuids_map[n] = o;
        console.log("-> Saved " + i + "\n\t by uuid " + uuid);
    }
    copy_actions(uuids_map);
    console.log("Saved actions count is: " + actions_count);
    console.log("--- End of save actions ---");
};

var init = function() {
    switch (process.argv[2]) {
        case "-save" : {
            save_actions();
            break;
        } case "-get" : {
            get_actions();
            break;
        } default : {
            console.log("Usage of script is: \tnodejs migrate.js[-save,-get]" +
                        "\n\t-save: Saves actions to leviathan storage," +
                        "\n\t-get: Gets actions from leviathan storage" +
                        "\n\tthen, merges and lints into actions.json file.");
            break;
        }
    }
};

init();
