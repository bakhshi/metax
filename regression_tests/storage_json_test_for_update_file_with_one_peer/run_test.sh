#!/bin/bash

# Test case flow:
# - Runs one instance of metax (storage size limit 0.2 GB ~ 200MB).
# - Generates a 10MB random file.
# - Sends "data" save request for generated file with enc=0 parameter.
# - Checks the validation of storage.json
# - Sends "data" update request for saved data.
# - Checks the validation of storage.json

save_file=$test_dir/save_file
updated_file=$test_dir/updated_file
get_file=$test_dir/get_file

source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0


main()
{
        $EXECUTE -f $tst_src_dir/config.xml &
        p=$!
        wait_for_init 7081
        echo -n $p > $test_dir/processes_ids
        uj_uuid=$(decrypt_user_json_info $test_dir/keys | jq '.uuid' | sed -e 's/^"//' -e 's/"$//')

        #Generated 10MB random file
        dd if=/dev/urandom of=$save_file bs=10000000 count=1
        dd if=/dev/urandom of=$updated_file bs=10000000 count=2
        uuid=$(curl --form "fileupload=@$save_file" http://localhost:7081/db/save/data?enc=0)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        curl http://localhost:7081/db/get/?id=$uuid --output $get_file
        sleep 2
        diff=$(diff $save_file $get_file 2>&1)
        if [ -n "$diff" ]; then
                echo "GET saved data failed."
                kill_metax $p
                exit;
        fi

        # storage size limit specified in GB. Getting size-limit from config.xml file
        size_limit=$(grep "<size_limit>" $tst_src_dir/config.xml | sed 's/ //g;s/<size_limit>//g;s/<\/size_limit>//g' | tr . ,)
        size_limit=$(($size_limit*1024*1024*1024))
        echo "size limit = $size_limit"

        storage_dir_size=$(wc -c $(find $test_dir/storage/????????-*) | tail -1 | awk {'print $1'})
        #storage_size=$(cat $test_dir/storage/storage.json | jq '.storage_size')
        cache_size=$(cat $test_dir/storage/storage.json | jq '.cache_size')
        echo "cache_size = $cache_size, storage_dir_size = $storage_dir_size"
        if [[ 0 != $cache_size || $storage_dir_size -gt $size_limit ]]; then
                echo "Invalid properties in storage.json or storage size greater than storage size limit"
                echo "cache_size = $cache_size, storage_dir_size = $storage_dir_size"
                kill_metax $p
                exit
        fi

        updated_uuid=$(curl --form "fileupload=@$updated_file" http://localhost:7081/db/save/data?id=$uuid\&enc=0)
        sleep 2
        updated_uuid=$(echo $updated_uuid | cut -d'"' -f 4)
        storage_dir_size1=$(wc -c $(find $test_dir/storage/????????-*) | tail -1 | awk {'print $1'})
        cache_size1=$(cat $test_dir/storage/storage.json | jq '.cache_size')
        echo "cache_size1 = $cache_size1, storage_dir_size1 = $storage_dir_size1"
        if [[ 0 != $cache_size1 || $storage_dir_size1 -gt $size_limit ]]; then
                echo "Invalid properties in storage.json or storage size1 greater than storage size limit"
                echo "cache_size1 = $cache_size1, storage_dir_size1 = $storage_dir_size1"
                kill_metax $p
                exit
        fi

        curl http://localhost:7081/db/get/?id=$updated_uuid --output $get_file
        diff=$(diff $updated_file $get_file 2>&1)
        if [ -n "$diff" ]; then
                echo "GET updated data failed."
                kill_metax $p
                exit;
        fi

        echo $PASS > $final_test_result
        kill_metax $p
}

main $@
