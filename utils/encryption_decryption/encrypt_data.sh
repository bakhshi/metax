#!/bin/bash

if [[ $# -ne 3 ]]
then
        echo "Please specify aes key and aes iv (both base64 encoded) and path to uuid"
        exit 1
fi

if [ "$(uname)" == "Darwin" ]; then
        base64_options="-D"
else
        base64_options="-d"
fi

openssl enc -e -aes256 -K `echo $1 | base64 $base64_options |  hexdump -e '16/1 "%02x"'`  -iv `echo $2 | base64 $base64_options |  hexdump -e '16/1 "%02x"'` -in $3
