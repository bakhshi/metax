#!/bin/bash

# Test case flow:
# - Runs and connects two instances of metax.
# - Generates a random file.
# - Sends "data" save request for generated file with enc=0 parameter.
# - Waits about 10 second for backup then sends update request with a
#   new generated file.
# - Waits about 15 second then kills all running metaxes.
# - Runs metax with config2.xml configuration.
# - Sends get request for received uuid to the running metax.
# - Checks the compliance of received and last generated files.

save_file=$test_dir/save_file
update_file=$test_dir/update_file
get_file=$test_dir/get_file
peers_count=0

source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0


clean_storage_content()
{
        echo "remove uuids(except user jsons) from $1 directory"
        str=""
        for i in ${!uj_uuids[@]}; do
                str+=" -o -name ${uj_uuids[$i]}"
        done
        find $1 -type f ! \( -name storage.json $str \) -delete
}




start_metaxes()
{
        $EXECUTE -f $tst_src_dir/config2.xml &
        p2=$!
        is_peer_connected=$(peers_connect 7091 $tst_src_dir/config2.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7090 does not connect to peers"
                kill_metax $p2
                exit;
        fi

        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7080 does not connect to peers"
                kill_metax $p1 $p2
                exit;
        fi
        echo -n $p2 $p1 > $test_dir/processes_ids
}


main()
{
        start_metaxes
        get_user_json_uuids 2
        dd if=/dev/urandom of=$save_file bs=16000000 count=1
        uuid=$(curl --form "fileupload=@$save_file" http://localhost:7081/db/save/data?enc=0)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        sleep $TIME
        dd if=/dev/urandom of=$update_file bs=16000000 count=1
        res=$(curl --form "fileupload=@$update_file" http://localhost:7081/db/save/data?id=$uuid\&enc=0)
        uuid1=$(echo $res | cut -d'"' -f 4)
        if [ "$uuid" == "$uuid1" ]; then
                sleep $TIME
                curl http://localhost:7081/db/get?id=$uuid1 --output $get_file
                d=$(diff $update_file $get_file 2>&1)
                if [[ -n $d ]]; then
                        echo "GET request failed."
                        kill_metax $p1 $p2
                        exit;
                fi

                kill_metax $p1 $p2
                clean_storage_content $test_dir/storage1
                start_metaxes

                curl http://localhost:7081/db/get?id=$uuid1 --output $get_file
                d=$(diff $update_file $get_file 2>&1)
                if [[ -n $d ]]; then
                        echo "GET request failed after removing storage1."
                        kill_metax $p1 $p2
                        exit;
                fi

                echo $PASS > $final_test_result
        fi
        kill_metax $p1 $p2
}

main $@
