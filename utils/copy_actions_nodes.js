var dbDir = process.argv[2] || '/home/employee/.leviathan/storage/';
var pure_storage = 'actions_' + Date.now() + '/';

var fs = require('fs');
if (! fs.existsSync(pure_storage)) {
    fs.mkdirSync(pure_storage.toString());
}

var fs = require('fs');
var actions = [
      "00000000-0000-0000-0000-222222222222"
    , "00000000-0000-0000-0000-333333333333"
    , "00000000-0000-0000-0000-444444444444"
    , "00000000-0000-0000-0000-555555555555"
    , "00000000-0000-0000-0000-666666666666"
    , "00000000-0000-0000-0000-777777777777"
    , "00000000-0000-0000-0000-888888888888"
    , "00000000-0000-0000-0000-000000000009"
    , "00000000-0000-0000-0000-000000000010"
    , "00000000-0000-0000-0000-000000000011"
    , "00000000-0000-0000-0000-111111111111"
    , "ffffffff-ffff-ffff-ffff-ffffffffffff"
];
function copyFile(source, target, cb) {
    var cbCalled = false;
    var rd = fs.createReadStream(source);
    rd.on("error", function(err) {
        done(err);
    });
    var wr = fs.createWriteStream(target);
    wr.on("error", function(err) {
        done(err);
    });
    wr.on("close", function(ex) {
        done();
    });
    rd.pipe(wr);
    function done(err) {
        if (!cbCalled) {
            //cb(err);
            cbCalled = true;
        }
    }
};
function init() {
    console.log("Begin of script");
    console.log("DB contains: " + actions.length + " actions");
    var new_db = [];
    for(var i = 0; i < actions.length; i++) {
        console.log("Master file is: " + dbDir + actions[i]);
        copyFile(dbDir + actions[i], pure_storage + actions[i]);
        new_db.push(JSON.parse(fs.readFileSync(dbDir + actions[i], 'utf8')));
        for(var key in new_db[i].pieces) {
            console.log("Piece is: " + key);
            copyFile(dbDir + key, pure_storage + key);
        }
    }
    console.log("End of script");
}
init();

