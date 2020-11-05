#!/bin/bash

# Test case flow: 
# - Runs one instance of metax.
# - Sends "path" save request for some data with enc=0.
# - Removes master json file from storage.
# - Sends "path" update request for received uuid with a new generated file.
# - Sends "get" request for received uuid.
# - Checks the compliance of received/updated datas.

save_file=$test_dir/save_file
update_file=$test_dir/update_file
get_file=$test_dir/get_file
rm -rf $test_dir/storage


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0



main()
{
        cd $test_dir
        $EXECUTE -f $tst_src_dir/config.xml &
        p=$!
        wait_for_init 7081
        echo -n $p > $test_dir/processes_ids

        dd if=/dev/urandom of=$save_file bs=16000000 count=1
        uuid=$(curl --data "$save_file" http://localhost:7081/db/save/path?enc=0)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        rm $test_dir/storage/$uuid
        dd if=/dev/urandom of=$update_file bs=16000000 count=1
        res=$(curl --data "$update_file" http://localhost:7081/db/save/path?id=$uuid\&enc=0)
        expected="{\"error\":\"Failed updating uuid=$uuid. UUID is not found.\"}"
        if [ "$expected" == "$res" ]; then
                echo $PASS > $final_test_result
        fi
        kill_metax $p
}

main $@
