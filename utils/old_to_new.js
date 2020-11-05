// Ports Vostan database into into Metax storage.
// The script operates on database acquired from export into html function. The
// config.js file generated with export should be cleaned up. Only json
// representing database should be left in the file.
// 
// Prerequisites synch-request and fs nodejs modules
// Metax web service should be up and running on 7071 port
//
// Usage: nodejs old_to_new.js <old_db>
// 	  <old_db> - html exported vostan folder where assets/config.js should
// 	  	     be cleaned up to contain only json database

NewJsonFormat =
{
    "id": 0,
    "domain": "",
    "uuid": "",
    "view": "",
    "title": {
        "value": "",
        "view": ""
    },
    "txt": {
        "value": "",
        "view": ""
    },
    "img": {
        "value": "",
        "view": ""
    },
    "script": "",
    "tags": [ ],
    "in": [ ],
    "out": [
    {
        "node" : { 
            "domain": "",
            "id": 0
        },
        "tag" : ""
    }
    ],
    "layout": [ ],
    "enc": 0 ,
    "created" : 0,
    "updated" : 0,
    "saved" : 0,
    "deleted" : 0
};

// Old db folder must be supplied, otherise print usage and exit
if (3 > process.argv.length) {
    console.log("Usage:\tnodejs old_to_new.js <old_db>");
    console.log("\t<old_db> - html exported vostan folder where assets/config.js should be");
    console.log("\t           cleaned up to contain only database json");
    process.exit(1);
}

old_db_folder = process.argv[2];
new_db_folder = "new_db/";

fs = require('fs');
request = require('sync-request');
var path = require("path");

// Gets all images used in the Vostan db and saves them in metax. The uuids of
// images is kept for setting up later in json files of corresponding nodes.
save_images = function(data) {
    var req = 'http://localhost:7071/db/save/path/';
    var db_path = '/var/www/html/instigate.am/';
    var image_json = {};
    for (var n in data) {
        root = data[n];
        for (var m in root['nodes']) {
            if (root['root'] == root['nodes'][m]['nodeID']) {
		rootImg = root['nodes'][m]['img']; 
		if (rootImg != null && rootImg != '' &&
				rootImg.substring(0, 14) != '../vostan_auth') {
		    var res = request("post", req, { body: db_path + rootImg });
		    var uuid = JSON.parse(res.body).uuid;
		    image_json[rootImg] = uuid;
		    console.log(rootImg);
		    console.log("created node: " + uuid);
		}
            }
        }
    }
    fs.writeFileSync(new_db_folder + "images.json", JSON.stringify(image_json));
    return image_json;
}

// Generate json files for each node with the Vostan P2P json format
// In resulting json files references to other nodes are still old node IDs,
// but references to images are metax uuids
port_db = function(data, images) {
    db_json = {};
    for (var n in data) {
        root = data[n];
        rootID = root['root']
        var out = []
        var inEdges = []
        var layout = []
        for (var m in root['nodes']) {
            nodeID = root['nodes'][m]['nodeID'];
            if (rootID == nodeID) {
                rootNode = root['nodes'][m]
            } else {
                node = { "domain": "", "id" : parseInt(nodeID) }
                for (var l in root['links']) {
                    links = root['links'][l];
                    if (links['nodeID'] == rootID &&
                        links['linkedNodeID'] == nodeID) {
                            tag = links['linkTags'];
			    tags = tag.split(",");
			    for (var p in tags) {
				    var itemOut = {
					    "node" : node,
					    "tag" : tags[p] + ''
				    }
				    out.push(itemOut)
			    }
		    }
                    if (links['nodeID'] == nodeID &&
                        links['linkedNodeID'] == rootID) {
                            tag = links['linkTags'];
			    tags = tag.split(",");
			    for (var p in tags) {
				    var itemIn = {
					    "node" : node,
					    "tag" : tags[p] + ''
				    }
				    inEdges.push(itemIn)
			    }
		    }
                }
                view = "1:" +   root['nodes'][m]['left'] + ":" + 
                                root['nodes'][m]['top'] + ":" + 
                                root['nodes'][m]['width'] + ":" + 
                                root['nodes'][m]['height'];

                title = {
                            "view" : root['nodes'][m]['titleInclude'] + ":" +
                            root['nodes'][m]['titleLeft'] + ":" +
                            root['nodes'][m]['titleTop'] + ":" +
                            root['nodes'][m]['titleWidth'] + ":" +
                            root['nodes'][m]['titleHeight']
                }
                txt = 	{
                            "view" : root['nodes'][m]['txtInclude'] + ":" +
                            root['nodes'][m]['txtLeft'] + ":" +
                            root['nodes'][m]['txtTop'] + ":" +
                            root['nodes'][m]['txtWidth'] + ":" +
                            root['nodes'][m]['txtHeight']
                };
                img = 	{
                            "view" : root['nodes'][m]['imgInclude'] + ":" +
                            root['nodes'][m]['imgLeft'] + ":" +
                            root['nodes'][m]['imgTop'] + ":" +
                            root['nodes'][m]['imgWidth'] + ":" +
                            root['nodes'][m]['imgHeight']
                };

                itemLayout = {
                                "node" : node,
                                "view" : view,
                                "title" : title,
                                "txt" : txt,
                                "img" : img,
                                "leaf" : parseInt(root['nodes'][m]['leaf'])
                }
                layout.push(itemLayout);
            }
        }
	db_json[rootID] = "";
        NewJsonFormat['id'] = rootID;
        NewJsonFormat['uuid'] = rootID;
        NewJsonFormat['view'] = "1:" +  rootNode['left'] + ":" + 
                                        rootNode['top'] + ":" + 
                                        rootNode['width'] + ":" + 
                                        rootNode['height'];
        title = {
            "value" :   rootNode['title'],
            "view" :    rootNode['titleInclude'] + ":" +
                        rootNode['titleLeft'] + ":" +
                        rootNode['titleTop'] + ":" +
                        rootNode['titleWidth'] + ":" +
                        rootNode['titleHeight']
        };
        txt = {
            "value" :   rootNode['txt'],
            "view" :    rootNode['txtInclude'] + ":" +
                        rootNode['txtLeft'] + ":" +
                        rootNode['txtTop'] + ":" +
                        rootNode['txtWidth'] + ":" +
                        rootNode['txtHeight']
        };
        img = {
            "value" :   images[rootNode['img']], 
            "view" :    rootNode['imgInclude'] + ":" +
                        rootNode['imgLeft'] + ":" +
                        rootNode['imgTop'] + ":" +
                        rootNode['imgWidth'] + ":" +
                        rootNode['imgHeight']
        };
        NewJsonFormat['title'] = title;
        NewJsonFormat['txt'] = txt;
        NewJsonFormat['img'] = img;
        NewJsonFormat['tags'] = [ ]
        tags = rootNode['tags'].split(",");
        for (var p in tags) {
                NewJsonFormat['tags'].push(tags[p] + '');
        }
        NewJsonFormat['in'] = inEdges;
        NewJsonFormat['out'] = out;
        NewJsonFormat['layout'] = layout;
        NewJsonFormat['enc'] = "0";
        timeNow = Date.now();
        NewJsonFormat['created'] = timeNow;
        NewJsonFormat['updated'] = timeNow;
        NewJsonFormat['saved'] = timeNow;

        fs.writeFileSync(new_db_folder + rootID + ".json", JSON.stringify(NewJsonFormat));
    }
    fs.writeFileSync(new_db_folder+ "db.json", JSON.stringify(db_json));
}

// Saves new jsons in metax and constructs map of old nodes IDs to metax UUIDs.
// With second pass replaces node IDs with UUIDs and updates metax storage
var update_db = function() {
    var db_json = fs.readFileSync(new_db_folder + 'db.json', 'utf8');
    db_json = JSON.parse(db_json);
    var req = 'http://localhost:7071/db/save/path/?enc=0';
    var db_path = path.resolve(new_db_folder) + '/';
    for (var n in db_json) {
            var res = request("post", req,
        		    { body: db_path + n + '.json'
        		    }
        		    );
            var uuid = JSON.parse(res.body).uuid;
            db_json[n] = uuid;
            console.log("saving node " + n + " with uuid " + uuid);
    }
    for (var n in db_json) {
	node = fs.readFileSync(new_db_folder + n + '.json', 'utf8');
	node = JSON.parse(node);
	node['id'] = db_json[n];
	node['uuid'] = db_json[n];
	for (var l in node['layout']) {
		var i = node['layout'][l]['node'];
		i['id'] = db_json[i['id']];
	}
	for (var l in node['out']) {
		var i = node['out'][l]['node'];
		i['id'] = db_json[i['id']];
	}
	for (var l in node['in']) {
		var i = node['in'][l]['node'];
		i['id'] = db_json[i['id']];
	}
	fs.writeFileSync(new_db_folder + n + '.json', JSON.stringify(node));

	req = 'http://localhost:7071/db/save/path/?enc=0&id=' + db_json[n];
	var res = request("post", req,
			{ body: db_path + n + '.json'
			}
			);
	var uuid = JSON.parse(res.body).uuid;
	console.log("updated node " + uuid);
    }
    fs.writeFileSync(new_db_folder + "db.json", JSON.stringify(db_json));
}


var old_db = fs.readFileSync(old_db_folder + '/assets/config.js', 'utf8');
old_db = JSON.parse(old_db);
var images = save_images(old_db);
var new_db = port_db(old_db, images);
//var images = fs.readFileSync('new_db/images.json', 'utf8');
//var new_db = port_db(old_db, JSON.parse(images));
update_db();

