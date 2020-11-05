#!/bin/bash

# Test case flow: 
# - Runs one instance of metax.
# - Sends "data" save request for a random generated file with enc=0.
# - Removes master json file from storage
# - Sends "data" update request for received uuid.
# - Sends get request for received uuid.
# - Checks the compliance of received/updated datas.

save_file=$test_dir/save_file
get_file=$test_dir/get_file


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
        uuid=$(curl --form "fileupload=@$save_file" http://localhost:7081/db/save/data?enc=0)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        rm $test_dir/storage/$uuid
        dd if=/dev/urandom of=$save_file bs=16000000 count=1
        res=$(curl --form "fileupload=@$save_file" http://localhost:7081/db/save/data?id=$uuid\&enc=0)
        expected="{\"error\":\"Failed updating uuid=$uuid. UUID is not found.\"}"
        echo "Res=$res"
        if [ "$expected" == "$res" ]; then
                echo $PASS > $final_test_result
        fi
        kill_metax $p
}

main $@
