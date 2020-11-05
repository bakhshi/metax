
On Mac OSX and Linux
Compilation:
$ /Users/garik/Library/Android/sdk/ndk-bundle/ndk-build

cp to board (password - 42)
$ scp -oKexAlgorithms=+diffie-hellman-group1-sha1 libs/armeabi-v7a/metax_web_api root@<ip>:/sdcard/
$ scp -oKexAlgorithms=+diffie-hellman-group1-sha1 <config.xml> root@<ip>:/sdcard/
Note, update config.xml file, e.g. storage and keys directory settings
might be the following:
<storage_path>/mnt/sata/8_2/leviathan/storage</storage_path>
<key_path>/mnt/sata/8_2/leviathan/keys/user</key_path>
<key_path>/mnt/sata/8_2/leviathan/keys/device</key_path>


login to board
$ cp /sdcard/metax_web_api /system/bin/
$ cp /sdcard/config.xml /mnt/sata/8_2/leviathan/config.xml
$ metax_web_api -f /mnt/sata/8_2/leviathan/config.xml

automatic start upon boot
adding metax_web_api executable to the following script will make it to start
upon android boot.
system/bin/remoteaccess.sh
TODO: refine instructions
