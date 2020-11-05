#!/bin/bash

if [[ $# -ne 3 ]]
then
        echo "Usage:"
        echo "$0 <private_key_path> <user_json_info> <out_file>"
        exit 1
fi

echo "leviathan" | openssl rsautl -decrypt -inkey $1 -in $2 -out $3 -passin stdin
