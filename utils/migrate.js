// The script converts Vostan database into into Metax storage.
// Prerequisites:
//   NodeJS (^4)
//   npm modules: sqlite3, sync-request, fs, path, child_process
//
// All configs can be changed from the global variables list.
// Usage: nodejs migrate.js [-cdb/-svgtopng/-si/-cn/-sn/-un/-all]
// Where:
// -cdb : convert data base
// -svgtopng : convert svg files to png files via inkscape
// -si : save images (resources)
// -cn : convert nodes
// -sn : save nodes into leviathan
// -un : update nodes (updates refered resources names to appropriate uuids)
// -all : do all mentioned necessary steps together

//Global variables
var request = require('sync-request');
var path = require('path'); 
var fs = require('fs');
//TODO: Add checker for convert paths
var exec = require("child_process").execSync;

var vostanDBFile = process.cwd() + '/vostan.db';
var assetsPath = process.cwd() + '/assets/';
var newDBPath = process.cwd() + '/new_db/';

var linksFile = newDBPath + "links.json";
var nodesFile = newDBPath + "nodes.json";
var settingsFile = newDBPath + "settings.json";
var resourcesFile = newDBPath + "resources.json";
var imagesFile = newDBPath + "images.json";
var dbFile = newDBPath + "db.json";
var dbUpdatedFile = newDBPath + "db_new.json";
//Map of node id with appropriate uuid
var mapFile = newDBPath + "map.json";
var db = null;
var host = 'http://localhost:7071/';
var uriSavePath = 'db/save/path?enc=0';
var uriSaveNode = 'db/save/node?enc=0';

//Global functions

var open_DB = function() {
    console.log("--- Start of Open DB ---");
    var data = {};
    var sqlite3 = require('sqlite3').verbose();
    db = new sqlite3.Database(vostanDBFile);
    if (! db) {
        data.error = "No Valid DB";
        handler.apply(null, [data]);
        return;
    }
    console.log("--- End of Open DB ---");
}

var getLinksCmd = function () {
    var linksCmd = "SELECT "
        + " l.nodeID         AS 'nodeID'"
        + ", l.linkedNodeID  AS 'linkedNodeID'"
        + ", l.tags          AS 'tags'"
        + ", l.tags_hy       AS 'tags_hy'"
        + ", l.tags_ru       AS 'tags_ru'"
        + ", l.modified      AS 'modified'"
        + "FROM links AS l";
    return linksCmd;
};

var getNodesCmd = function () {
    var nodesCmd = "SELECT "
        + " n.nodeID          AS 'nodeID'"
        + ", n.title          AS 'title'"
        + ", n.title_hy       AS 'title_hy'"
        + ", n.title_ru       AS 'title_ru'"
        + ", n.img            AS 'img'"
        + ", n.txt            AS 'txt'"
        + ", n.txt_hy         AS 'txt_hy'"
        + ", n.txt_ru         AS 'txt_ru'"
        + ", n.tags           AS 'tags'"
        + ", n.tags_hy        AS 'tags_hy'"
        + ", n.tags_ru        AS 'tags_ru'"
        + ", n.script         AS 'script'"
        + ", n.location       AS 'location'"
        + ", n.users          AS 'users'"
        + ", n.viewers        AS 'viewers'"
        + ", n.modified       AS 'modified'"
        + ", n.carousel       AS 'carousel'"
        + ", n.isDefaultRoot  AS 'isDefaultRoot'"
        + " FROM nodes AS n";
    return nodesCmd;
};

var getSettingsCmd = function () {
    var settingsCmd = "SELECT "
        + " s.nodeID         AS 'nodeID'"
        + ", s.linkedNodeID  AS 'linkedNodeID'"
        + ", s.radius        AS 'radius'"
        + ", s.top           AS 'top'"
        + ", s.left          AS 'left'"
        + ", s.width         AS 'width'"
        + ", s.height        AS 'height'"
        + ", s.imgWidth      AS 'imgWidth'"
        + ", s.imgHeight     AS 'imgHeight'"
        + ", s.imgLeft       AS 'imgLeft'"
        + ", s.imgTop        AS 'imgTop'"
        + ", s.titleWidth    AS 'titleWidth'"
        + ", s.titleHeight   AS 'titleHeight'"
        + ", s.titleLeft     AS 'titleLeft'"
        + ", s.titleTop      AS 'titleTop'"
        + ", s.txtWidth      AS 'txtWidth'"
        + ", s.txtHeight     AS 'txtHeight'"
        + ", s.txtLeft       AS 'txtLeft'"
        + ", s.txtTop        AS 'txtTop'"
        + ", s.imgInclude    AS 'imgInclude'"
        + ", s.titleInclude  AS 'titleInclude'"
        + ", s.txtInclude    AS 'txtInclude'"
        + ", s.modified      AS 'modified'"
        + ", s.leaf          AS 'leaf'"
        + ", s.carousel      AS 'carousel'"
        + " FROM settings AS s";
    return settingsCmd;
};

var convert_SQLite_to_JSON = function() {
    console.log("--- Start of convert SQlite vostan db to JSON ---");
    var dbExists = fs.existsSync(vostanDBFile);
    if (! dbExists) {
        console.log('Can not find the ' + vostanDBFile);
        return;
    }
    open_DB();
    var linksCmd = getLinksCmd();
    var nodesCmd = getNodesCmd();
    var settingsCmd = getSettingsCmd();
    var links = [];
    var nodes = [];
    var settings = [];
    var resources = [];
    db.serialize(function() {
        db.each(linksCmd, function(err, row) {
            if (err) {
                console.log("Error in get links " + err);
            } else {
                links.push(row);
                fs.writeFileSync(linksFile, JSON.stringify(links));
            }
        });
        db.each(nodesCmd, function(err, row) {
            if (err) {
                console.log("Error in get nodes " + err);
            } else {
                if (row.img && null != row.img && '' != row.img) {
                    resources.push(row.img);
                }
                nodes.push(row);
                fs.writeFileSync(resourcesFile, JSON.stringify(resources));
                fs.writeFileSync(nodesFile, JSON.stringify(nodes));
            }
        });
        db.each(settingsCmd, function(err, row) {
            if (err) {
                console.log("Error in get settings " + err);
            } else {
                settings.push(row);
                fs.writeFileSync(settingsFile, JSON.stringify(settings));
            }
        });
    });
    db.close();
    console.log("--- End of SQlite vostan db to JSON ---");
};

//Converts svg to png by inkscape, need to have already installed it. 
//CMD "inkscape --export-png=a.png  --export-area-drawing -d 270 b.svg"
var convert_svg_to_png = function() {
    var res = JSON.parse(fs.readFileSync(resourcesFile, 'utf8'));
    for (var i = 0; i < res.length; i++) {
        var fsvg = assetsPath + res[i];
        console.log("Checking image " + fsvg);
        if (fs.existsSync(fsvg) && -1 != res[i].indexOf('svg')) {
            console.log("Converting " + res[i]);
            var cmd = "inkscape --export-png=" + assetsPath + res[i].replace(
                      ".svg", ".png") + " --export-area-drawing -d 270 " + fsvg;
            var c = exec(cmd);
            console.log("Executed command: " + cmd + "\n\tCompleted: " + c);
        }
    }
};

// Gets all images used in the Vostan db and saves them in metax. The uuids of
// images is kept for setting up later in json files of corresponding nodes.
var save_images = function(data) {
    console.log("--- Start of save images ---");
    var req = host + uriSavePath;
    var images = {};
    var images_count = 0;
    console.log('-> Saving image url is: ' + req + '\n');
    for (var i = 0; i < data.length; i++) {
        var img = data[i].img;
        console.log('-> Ckecking image: ' + assetsPath + img);
        if (! images[img] && '' != images[img] && null != img && '' != img
             && -1 == img.indexOf('vostan_auth')
             && -1 == img.indexOf('attachments')
             && fs.existsSync(assetsPath + img)) {
            img = (-1 != img.indexOf('svg')) ? img.replace('.svg', '.png') : img;
            data[i].img = img;
            console.log('-> Saving ' + img + ' by id ' + i);
            var res = request('post', req, { body: assetsPath + img });
            images[img] = JSON.parse(res.body)['uuid'];
            images_count++;
            console.log('-> Saved ' + images_count + ' -th image, id is: '
                + i + ', title is: ' + img + ', uuid is: ' + images[img]);
        }
    }
    console.log("Saved resources count in leviathan is: " + images_count);
    fs.writeFileSync(imagesFile, JSON.stringify(images));
    fs.writeFileSync(nodesFile, JSON.stringify(data));
    console.log("--- End of save images ---");
};

var is_null = function(k) {
    return ('undefined' === k || null === k);
};

var get_localization_data = function(o) {
    var keys = Object.keys(o);
    var n = {'title': {}, 'img': {}, 'txt': {}, 'tags': []};
    //var n = {'title': {}, 'img': {}, 'txt': {}, 'tags': {}};
    var v = 'value';
    var k = '';
    for (var i = 0; i < keys.length; i++) {
        k = keys[i];
        if (-1 != k.indexOf('title')) {
            n.title[v + k.split('title')[1]] = is_null(o[k]) ? '' : o[k];
        } else if (-1 != k.indexOf('img')) {
            n.img[v + k.split('img')[1]] = is_null(o[k]) ? '' : o[k];
        } else if (-1 != k.indexOf('txt')) {
            n.txt[l = v + k.split('txt')[1]] = is_null(o[k]) ? '' : o[k];
        } else if (-1 != k.indexOf('tags')) {
            n.tags.push(is_null(o[k]) ? '' : o[k]);
            //n.tags[v + k.split('tags')[1]] = is_null(o[k]) ? o[k] : '';
        }
    }
    n["script"] = (! is_null(o['script'])) ? o['script'] : '';
    n["created"] = (! is_null(o['modified'])) ? o['modified'] : Date.now();
    n["viewers"] = (! is_null(o['viewers'])) ? o['viewers'] : '';
    return n;
};

var get_node_attr = function(obj) {
    var n = get_localization_data(obj);
    n["id"] = ! is_null(obj) && obj['nodeID'] ? obj['nodeID'] : 0;
    n["domain"] = '';
    n["enc"] = '0';
    n["view"] = '';
    n["title"]["view"] = '';
    n["img"]["view"] = '';
    n["txt"]["view"] = '';
    n["editors"] = '';
    n["updated"] = 0;
    n["saved"] = 0;
    n["deleted"] = 0;
    n["moved"] = {"date": 0, "domain": '', "id": ''};
    return n;
};

var get_node_out = function(links, id) {
    var l = {'out': [], 'in': []};
    for (var i = 0; i < links.length; i++) {
        if (links[i].nodeID == id || links[i].linkedNodeID == id) {
            var edge = {
                "node": { "id": "", "domain": "" },
                "tag": (links[i].tags && null != links[i].tags) ?
                        links[i].tags : ''
            };
            if (links[i].nodeID == id) {
                edge.node["id"] = links[i].linkedNodeID;
                l.out.push(edge);
            } else if (links[i].linkedNodeID == id) {
                edge.node["id"] = links[i].nodeID;
                l.in.push(edge);
            }
        }
    }
    return l;
};

var round = function(i) {
    return Math.round(i);
};

var get_node_layout_drawing_attr = function(s, id) {
    var node = {
        "view": "", 
        "leaf": 0, 
        "title": {
            "view": ""
        },
        "txt": {
            "view": ""
        },
        "img": {
            "view": ""
        },
        "layout": []
    };
    for (var i = 0; i < s.length; i++) {
        if (s[i].nodeID == id && s[i].linkedNodeID == id) {
            node.view = '1:' + round(s[i].left) + ':' + round(s[i].top)
                + ':' + round(s[i].width) + ':' + round(s[i].height);
            node.title.view = s[i].titleInclude + ':' + s[i].titleLeft + ':' +
                s[i].titleTop + ':' + s[i].titleWidth + ':' + s[i].titleHeight;
            node.img.view = s[i].imgInclude + ':' + s[i].imgLeft + ':' +
                s[i].imgTop + ':' + s[i].imgWidth + ':' + s[i].imgHeight;
            node.txt.view = s[i].txtInclude + ':' + s[i].txtLeft + ':' +
                s[i].txtTop + ':' + s[i].txtWidth + ':' + s[i].txtHeight;
            node.leaf = s[i].leaf;
        } else if (s[i].nodeID == id) {
            var l = {
                "node": {
                    "domain": "",
                    "id": s[i].linkedNodeID
                },
                "view": "1:" + round(s[i].left) + ":" + round(s[i].top)
                    + ":" + round(s[i].width) + ":" + round(s[i].height),
                "title": { "view": s[i].titleInclude + ":"
                    + round(s[i].titleLeft) + ":" + round(s[i].titleTop) + ":"
                    + round(s[i].titleWidth) + ":" + round(s[i].titleHeight)
                },
                "txt": { "view": s[i].txtInclude + ":"
                    + round(s[i].txtLeft) + ":" + round(s[i].txtTop) + ":"
                    + round(s[i].txtWidth) + ":" + round(s[i].txtHeight)
                },
                "img": { "view": s[i].imgInclude + ":"
                    + round(s[i].imgLeft) + ":" + round(s[i].imgTop) + ":"
                    + round(s[i].imgWidth) + ":" + round(s[i].imgHeight)
                },
                "leaf": s[i].leaf
            };
            node.layout.push(l);
        }
    }
    return node;
};

var convert_nodes_content = function(nodes, links, settings) {
    console.log("--- Start of convert nodes content to new ---");
    var db = [];
    for (var i = 0; i < nodes.length; i++) {
        var node = get_node_attr(nodes[i]);
        var l = get_node_out(links, node.id);
        var n = get_node_layout_drawing_attr(settings, node.id);
        node["view"] = n.view;
        node["title"]["view"] = n.title.view;
        node["txt"]["view"] = n.txt.view;
        node["img"]["view"] = n.img.view;
        node["leaf"] = n.leaf;
        node['in'] = l.in;
        node['out'] = l.out;
        node["layout"] = n.layout;
        db.push(node);
        var p = newDBPath + node.id + '.json';
        fs.writeFileSync(p, JSON.stringify(node));
        console.log("-> Converted and saved node : " + i + " -th, id is: "
                    + node.id + ", path is: " + p);
    }
    console.log("Converted nodes count is: " + nodes.length);
    fs.writeFileSync(dbFile, JSON.stringify(db));
    console.log("--- End of convert nodes content to new ---");
};

// Saves new jsons in metax and constructs map of old nodes IDs to metax UUIDs.
// With second pass replaces node IDs with UUIDs and updates metax storage
// Saves empty content to not create pieces. As this save is for getting
// some generated uuid for node. After what would be sent update with
// real content and with updated uuids in place of node id.
var save_nodes = function(db) {
    console.log("--- Start of save nodes ---");
    //Saves empty nodes in leviathan
    var req = host + uriSaveNode;
    var map = {};
    for (var i = 0; i < db.length; i++) {
        var node_path = newDBPath + db[i].id + '.json';
        console.log("-> Request uri is: " + req);
        var res = request("post", req, { body: '' });
        var uuid = JSON.parse(res.body).uuid;
        map[db[i].id] = uuid;
        console.log("-> Saving node " + i + " -th, id is: "
                    + db[i].id + ", uuid is: " + uuid);
    }
    console.log("Saved nodes count in leviathan is: " + db.length);
    fs.writeFileSync(mapFile, JSON.stringify(map));
    console.log("--- End of save nodes ---");
};

var update_nodes = function(db, map, images_map) {
    console.log("--- Start of updates nodes ---");
    //Updated node uuid instead of id
    var db_updated = [];
    for (var i = 0; i < db.length; i++) {
        var node = db[i];
        var id = node.id;
        node.id = map[node.id];
        console.log("-------img old " + node.img.value + ", new " + images_map[node.img.value]);
        node.img['value'] = images_map[node.img['value']];
        var out = node['out'];
        for (var m = 0; m < out.length; m++) {
            var oid = out[m].node['id'];
            if (oid) {
                out[m].node['id'] = map[oid];
            }
        }
        for (var j = 0; j < node['in'].length; j++) {
            var iid = node['in'][j].node['id'];
            if (iid) {
                node['in'][j].node['id'] = map[iid];
            }
        }
        for (var k = 0; k < node['layout'].length; k++) {
            var lid = node['layout'][k].node['id'];
            if (lid) {
                node['layout'][k].node['id'] = map[lid];
            }
        }
        db_updated.push(node);
        fs.writeFileSync(newDBPath + id + "_new.json", JSON.stringify(node));
        fs.writeFileSync(newDBPath + node.id, JSON.stringify(node));
    }
    console.log("Updated nodes (with uuids) count is: " + db.length);
    fs.writeFileSync(dbUpdatedFile, JSON.stringify(db_updated));
    //Updates nodes in leviathan 
    var req = host + uriSavePath;
    for (var i = 0; i < db_updated.length; i++) {
        var uri = req + '&id=' + db_updated[i].id;
        console.log (uri);
        var data = newDBPath + db_updated[i].id;
        console.log(data);
        var res = request("post", uri, { body: data });
        var uuid = JSON.parse(res.body).uuid;
        console.log("-> Updated node " + i + " by uuid: " + uuid);
    }
    console.log("Updated count in leviathan is: " + db_updated.length);
    console.log("--- End of updates nodes ---");
};

var prepare_folders = function() {
    if (! fs.existsSync(newDBPath)) {
         fs.mkdirSync(newDBPath)
    }
};

var start_all_convertings = function() {
    //Make sure that presents converted json of sqlite db
    var nodes = fs.readFileSync(nodesFile, 'utf8');
    nodes = JSON.parse(nodes);
    save_images(nodes);
    var nodes = JSON.parse(fs.readFileSync(nodesFile, 'utf8'));
    var links = JSON.parse(fs.readFileSync(linksFile, 'utf8'));
    var settings = JSON.parse(fs.readFileSync(settingsFile, 'utf8'));
    convert_nodes_content(nodes, links, settings);
    var db = JSON.parse(fs.readFileSync(dbFile, 'utf8'));
    save_nodes(db);
    var db = JSON.parse(fs.readFileSync(dbFile, 'utf8'));
    var map = JSON.parse(fs.readFileSync(mapFile, 'utf8'));
    var images = JSON.parse(fs.readFileSync(imagesFile, 'utf8'));
    update_nodes(db, map, images);
};

var migrate = function() {
    if (process.argv[2]) {
        prepare_folders();
    }
    switch (process.argv[2]) {
        case '-all' : {
            start_all_convertings();
            break;
        } case '-cdb' : {
            convert_SQLite_to_JSON();
            break;
        } case '-svgtopng' : {
            convert_svg_to_png(nodes);
            break;
        } case '-si' : {
            var nodes = fs.readFileSync(nodesFile, 'utf8');
            nodes = JSON.parse(nodes);
            save_images(nodes);
            break;
        } case '-cn' : {
            var nodes = JSON.parse(fs.readFileSync(nodesFile, 'utf8'));
            var links = JSON.parse(fs.readFileSync(linksFile, 'utf8'));
            var settings = JSON.parse(fs.readFileSync(settingsFile, 'utf8'));
            convert_nodes_content(nodes, links, settings);
            break;
        } case '-sn' : {
            var db = JSON.parse(fs.readFileSync(dbFile, 'utf8'));
            save_nodes(db);
            break;
        } case '-un' : {
            var db = JSON.parse(fs.readFileSync(dbFile, 'utf8'));
            var map = JSON.parse(fs.readFileSync(mapFile, 'utf8'));
            var images = JSON.parse(fs.readFileSync(imagesFile, 'utf8'));
            update_nodes(db, map, images);
            break;
        } default : {
            console.log("Usage of script is: \tnodejs migrate.js"
                        + "[-cdb/-svgtopng/-si/-cn/-sn/-un/-all]");
            //console.log("\tSet the 3-th argument for the preferable language.");
            break;
        }
    }
};

migrate();
