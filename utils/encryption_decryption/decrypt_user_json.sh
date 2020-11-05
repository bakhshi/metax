#!/bin/bash

if [[ $# -ne 4 ]]
then
        echo "Usage:"
        echo "$0 <user_private_key> <user_json_info> <storage_dir> <out_file>"
        echo or
        echo "$0 <user_private_key> <user_json_info> <user_json_file> <out_file>"
        exit 1
fi

#!/usr/bin/env bash

if [ "$(uname)" == "Darwin" ]; then
        base64_options="-D"
else
        base64_options="-d"
fi

decrypted_user_json_info=$(mktemp)

echo "leviathan" | openssl rsautl -decrypt -inkey $1 -in $2 -out $decrypted_user_json_info -passin stdin

iv=$(cat $decrypted_user_json_info | jq -r '.aes_iv')
#iv=$(cat $decrypted_user_json_info | cut -d'"' -f 4)
#eval iv=$iv

key=$(cat $decrypted_user_json_info | jq -r '.aes_key')
#key=$(cat $decrypted_user_json_info | cut -d'"' -f 8)
#eval key=$key

if [ -d $3 ]; then
        user_json_filename=$(cat $decrypted_user_json_info | jq -r '.uuid')
        #user_json_filename=$(cat $decrypted_user_json_info | cut -d'"' -f 12)
        #eval user_json_filename=$user_json_filename
        user_json_path=$3/$user_json_filename
else
        user_json_path=$3
fi

hex_iv=$(echo $iv | base64 $base64_options |  hexdump -e '16/1 "%02x"')

hex_key=$(echo $key | base64 $base64_options |  hexdump -e '16/1 "%02x"')

openssl enc -d -aes256 -K $hex_key  -iv $hex_iv -in $user_json_path | jq '.' > $4

rm $decrypted_user_json_info
