#!/bin/bash

curl http://localhost:7071/db/get/\?id\=62347c12-af62-4ce6-b2a1-d24c06807b81 > changelog.txt
line=$(head -n 1 changelog.txt)
version=$(/opt/leviathan/bin/metax -v)
ver () { printf "%03d%03d%03d%03d" $(echo "$1" | tr '.' '\n' | head -n 4); }
if [ `ver "$version"` -lt `ver "$line"` ]
then
        line1=$(sed '2q;d' changelog.txt)
        curl http://localhost:7071/db/get/\?id\=$line1 > update.tar.bz2
        tar -xf update.tar.bz2
        mv ./update/metax /opt/leviathan/bin/metax
        mv ./update/update.sh /opt/leviathan/bin/metax_update.sh
        mv ./update/config.xml /opt/leviathan/config.xml

        echo "Metax successfully updated!"
fi
