#!/bin/bash


export project_root=$PWD
source $project_root/test_utils/functions.sh

test_src_dir=regression_tests/valgrind_test_without_peers

export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
valgrind --leak-check=full --log-file=valgrind_result_save_update_wo_peer.txt ./bin/metax_web_api -f $test_src_dir/config.xml &
pid=$!
wait_for_init 7081

save_content="save content"
update_encrypted_content="updated encrypted content"
update_unencrypted_content="updated unencrypted content"
save_file_path=$test_src_dir/save_content
update_encrypted_path=$test_src_dir/update_encrypted_path
update_unencrypted_path=$test_src_dir/update_unencrypted_path

echo "################## TESTING SAVE AND UPDATE BY CONTENT #################"
echo unencrypted save by content
for (( i=0; c<7; c++ ))
do
        uuid=$(curl --data "$save_content" "http://localhost:7081/db/save/node?enc=0" 2> /dev/null)
        ss=$?
	# 7 status for curl means connection refused
        if [ $ss -ne 7 ]; then
                break
        fi
        sleep 3
done
if [ $ss -ne 0 ]; then
        echo "failed to connect to Metax"
        exit 1
fi
uuid=$(echo $uuid | cut -d'"' -f 4)
get_content=$(curl http://localhost:7081/db/get?id=$uuid 2> /dev/null)
echo $get_content - $uuid
if [ "$get_content" != "$save_content" ]
then
        echo "error saving file"
        kill $pid
        exit 1;
fi

echo unencrypted update over unencrypted data by content
curl --data "$update_unencrypted_content" "http://localhost:7081/db/save/node?enc=0&id=$uuid" &> /dev/null
get_content=$(curl http://localhost:7081/db/get?id=$uuid 2> /dev/null)
echo $get_content - $uuid
if [ "$get_content" != "$update_unencrypted_content" ]
then
        echo "error updating file"
        kill $pid
        exit 1;
fi

echo encrypted update over unencrypted data by content
curl --data "$update_encrypted_content" "http://localhost:7081/db/save/node?enc=1&id=$uuid" &> /dev/null
get_content=$(curl http://localhost:7081/db/get?id=$uuid 2> /dev/null)
echo $get_content - $uuid
if [ "$get_content" != "$update_encrypted_content" ]
then
        echo "error updating file"
        kill $pid
        exit 1;
fi

echo encrypted save by content
uuid=$(curl --data "$save_content" "http://localhost:7081/db/save/node?enc=1" 2> /dev/null)
uuid=$(echo $uuid | cut -d'"' -f 4)
get_content=$(curl http://localhost:7081/db/get?id=$uuid 2> /dev/null)
echo $get_content
if [ "$get_content" != "$save_content" ]
then
        echo "error saving file"
        kill $pid
        exit 1;
fi

echo encrypted update over encrypted data by content
curl --data "$update_encrypted_content" "http://localhost:7081/db/save/node?enc=1&id=$uuid" &> /dev/null
get_content=$(curl http://localhost:7081/db/get?id=$uuid 2> /dev/null)
echo $get_content - $uuid
if [ "$get_content" != "$update_encrypted_content" ]
then
        echo "error saving file"
        kill $pid
        exit 1;
fi

echo unencrypted update over encrypted data by content
curl --data "$update_unencrypted_content" "http://localhost:7081/db/save/node?enc=0&id=$uuid" &> /dev/null
get_content=$(curl http://localhost:7081/db/get?id=$uuid 2> /dev/null)
echo $get_content - $uuid
if [ "$get_content" != "$update_unencrypted_content" ]
then
        echo "error saving file"
        kill $pid
        exit 1;
fi

echo "################## TESTING SAVE AND UPDATE BY FILE #################"
echo unencrypted save by file
uuid=$(curl --form "fileupload=@$save_file_path" "http://localhost:7081/db/save/data?enc=0" 2> /dev/null)
uuid=$(echo $uuid | cut -d'"' -f 4)
get_content=$(curl http://localhost:7081/db/get?id=$uuid 2> /dev/null)
echo $get_content - $uuid
if [ "$get_content" != "$save_content" ]
then
        echo "error saving file"
        kill $pid
        exit 1;
fi

echo unencrypted update over unencrypted data by file
curl --form "fileupload=@$update_unencrypted_path" "http://localhost:7081/db/save/data?enc=0&id=$uuid" &> /dev/null
get_content=$(curl http://localhost:7081/db/get?id=$uuid 2> /dev/null)
echo $get_content - $uuid
if [ "$get_content" != "$update_unencrypted_content" ]
then
        echo "error updating file"
        kill $pid
        exit 1;
fi

echo encrypted update over unencrypted data by file
curl --form "fileupload=@$update_encrypted_path" "http://localhost:7081/db/save/data?enc=1&id=$uuid" &> /dev/null
get_content=$(curl http://localhost:7081/db/get?id=$uuid 2> /dev/null)
echo $get_content - $uuid
if [ "$get_content" != "$update_encrypted_content" ]
then
        echo "error updating file"
        kill $pid
        exit 1;
fi

echo encrypted save by file
uuid=$(curl --form "fileupload=@$save_file_path" "http://localhost:7081/db/save/data?enc=1" 2> /dev/null)
uuid=$(echo $uuid | cut -d'"' -f 4)
get_content=$(curl http://localhost:7081/db/get?id=$uuid 2> /dev/null)
echo $get_content - $uuid
if [ "$get_content" != "$save_content" ]
then
        echo "error saving file"
        kill $pid
        exit 1;
fi

echo encrypted update over encrypted data by file
curl --form "fileupload=@$update_encrypted_path" "http://localhost:7081/db/save/data?enc=1&id=$uuid" &> /dev/null
get_content=$(curl http://localhost:7081/db/get?id=$uuid 2> /dev/null)
echo $get_content - $uuid
if [ "$get_content" != "$update_encrypted_content" ]
then
        echo "error saving file"
        kill $pid
        exit 1;
fi

echo unencrypted update over encrypted data by file
curl --form "fileupload=@$update_unencrypted_path" "http://localhost:7081/db/save/data?enc=0&id=$uuid" &> /dev/null
get_content=$(curl http://localhost:7081/db/get?id=$uuid 2> /dev/null)
echo $get_content - $uuid
if [ "$get_content" != "$update_unencrypted_content" ]
then
        echo "error saving file"
        kill $pid
        exit 1;
fi

echo "################## TESTING SAVE AND UPDATE BY PATH #################"
echo unencrypted save by file
uuid=$(curl --data "$save_file_path" "http://localhost:7081/db/save/path?enc=0" 2> /dev/null)
uuid=$(echo $uuid | cut -d'"' -f 4)
get_content=$(curl http://localhost:7081/db/get?id=$uuid 2> /dev/null)
echo $get_content - $uuid
if [ "$get_content" != "$save_content" ]
then
        echo "error saving file"
        kill $pid
        exit 1;
fi

echo unencrypted update over unencrypted data by file
curl --data "$update_unencrypted_path" "http://localhost:7081/db/save/path?enc=0&id=$uuid" &> /dev/null
get_content=$(curl http://localhost:7081/db/get?id=$uuid 2> /dev/null)
echo $get_content - $uuid
if [ "$get_content" != "$update_unencrypted_content" ]
then
        echo "error updating file"
        kill $pid
        exit 1;
fi

echo encrypted update over unencrypted data by file
curl --data "$update_encrypted_path" "http://localhost:7081/db/save/path?enc=1&id=$uuid" &> /dev/null
get_content=$(curl http://localhost:7081/db/get?id=$uuid 2> /dev/null)
echo $get_content - $uuid
if [ "$get_content" != "$update_encrypted_content" ]
then
        echo "error updating file"
        kill $pid
        exit 1;
fi

echo encrypted save by file
uuid=$(curl --data "$save_file_path" "http://localhost:7081/db/save/path?enc=1" 2> /dev/null)
uuid=$(echo $uuid | cut -d'"' -f 4)
get_content=$(curl http://localhost:7081/db/get?id=$uuid 2> /dev/null)
echo $get_content - $uuid
if [ "$get_content" != "$save_content" ]
then
        echo "error saving file"
        kill $pid
        exit 1;
fi

echo encrypted update over encrypted data by file
curl --data "$update_encrypted_path" "http://localhost:7081/db/save/path?enc=1&id=$uuid" &> /dev/null
get_content=$(curl http://localhost:7081/db/get?id=$uuid 2> /dev/null)
echo $get_content - $uuid
if [ "$get_content" != "$update_encrypted_content" ]
then
        echo "error saving file"
        kill $pid
        exit 1;
fi

echo unencrypted update over encrypted data by file
curl --data "$update_unencrypted_path" "http://localhost:7081/db/save/path?enc=0&id=$uuid" &> /dev/null
get_content=$(curl http://localhost:7081/db/get?id=$uuid 2> /dev/null)
echo $get_content - $uuid
if [ "$get_content" != "$update_unencrypted_content" ]
then
        echo "error saving file"
        kill $pid
        exit 1;
fi

echo "SUCCESS"

kill $pid

