#!/bin/bash

# Test case flow:
# - Runs one instance of metax.
# - Generates a random file (file size greater than storage size limit).
# - Sends "data" save request for generated file.
# - Checks the validation of storage.json

save_file=$test_dir/save_file

source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0

main()
{
        $EXECUTE -f $tst_src_dir/config.xml &
        p=$!
        wait_for_init 7081
        echo -n $p > $test_dir/processes_ids

        #Generated 20MB random file
        dd if=/dev/urandom of=$save_file bs=10000000 count=2
        res=$(curl -D $test_dir/h --form "fileupload=@$save_file" http://localhost:7081/db/save/data?enc=0)
        echo "res = $res"
        kill_metax $p
        expected='{"error":"Failed to save."}'
        if [ "$res" != "$expected" ]; then
                echo "Mismatch of expected/received messages. Expected = $expected, res = $res"
                exit
        fi
        http_code=$(cat $test_dir/h | grep HTTP/1.1 | grep OK | awk {'print $2'})
        echo "HTTP code of SAVE request response is $http_code"
        if [[ 404 != $http_code ]]; then
                echo "HTTP code of SAVE request response is $http_code"
                exit
        fi

        storage_size=$(cat $test_dir/storage/storage.json | jq '.storage_size')
        cache_size=$(cat $test_dir/storage/storage.json | jq '.cache_size')
        if [[ $cache_size != 0 || $storage_size != 0 ]]; then
                echo "Invalid properties in stoprage.json. cache_size = $cache_size, storage_size = $storage_size"
                exit
        fi
        echo $PASS > $final_test_result
}

main $@
