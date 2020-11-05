#!/bin/bash

# Test caseflow:
# - Runs and connects two instances of metax (metax1 [size_limit = 0] -> metax2 [size_limit = 0.1]).
# - Generated 20 MB file
# - Sends "data" save request for generated file (enc = 0).
# - Waits about 10 second for backup save.
# - Sends copy request for saved data.
# - Waits about 10 second for backup copy.
# - Checks that data is copied in metax1.

source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0
save_file=$test_dir/save_file
get_file1=$test_dir/get_file1
get_file2=$test_dir/get_file2


start_metaxes()
{
        $EXECUTE -f $tst_src_dir/config2.xml &
        p2=$!
        is_peer_connected=$(peers_connect 7091 $tst_src_dir/config2.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7090 does not connect to peer"
                kill_metax $p2
                exit;
        fi

        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7080 does not connect to peers"
                kill_metax $p2 $p1
                exit;
        fi
        echo -n $p2 $p1 > $test_dir/processes_ids
}


main()
{
        start_metaxes
        get_user_json_uuids 2

        #Generated 20MB random file
        dd if=/dev/urandom of=$save_file bs=10000000 count=2
        res=$(curl --form "fileupload=@$save_file" http://localhost:7081/db/save/data?enc=0)
        uuid=$(echo $res | cut -d'"' -f 4)
        echo "uuid = $uuid"
        #Waits 10 second for backup save
        sleep $TIME

        uuid_exist=$(find $test_dir/storage2 -name $uuid | wc -l)
        storage_dir_size1=$(ls $test_dir/storage1/????????-* | grep -v "${uj_uuids[0]}" | grep -v "${uj_uuids[1]}" | expr $(wc -l))
        if [ $storage_dir_size1 != 0 ] || [ 1 != $uuid_exist ] ; then
                echo "File should not be saved in storage1. storage_dir_size1 = $storage_dir_size1"
                echo "$uuid uuid should be exist in storage2."
                kill_metax $p1 $p2
                exit;
        fi

        curl http://localhost:7091/db/get/?id=$uuid --output $get_file1
        diff=$(diff $save_file $get_file1 2>&1)
        if [ -n "$diff" ]; then
                echo "GET request failed."
                kill_metax $p1 $p2
                exit;
        fi

        copy_res=$(curl http://localhost:7081/db/copy?id=$uuid)
        echo "copy_res=$copy_res"
        copy_uuid=$(echo $copy_res | jq -r '.copy_uuid')
        orig_uuid=$(echo $copy_res | jq -r '.orig_uuid')
        if [ "$orig_uuid" != "$uuid" ]; then
                echo "Incorrect copy request response : $copy_res"
                kill_metax $p1 $p2
                exit;
        fi

        copy_uuid_exist=$(find $test_dir/storage2 -name $copy_uuid | wc -l)
        orig_uuid_exist=$(find $test_dir/storage2 -name $orig_uuid | wc -l)
        storage_dir_size1=$(ls $test_dir/storage1/????????-* | grep -v "${uj_uuids[0]}" | grep -v "${uj_uuids[1]}" | expr $(wc -l))
        if [ $storage_dir_size1 != 0 ] || [ 1 != $copy_uuid_exist ] || [ 1 != $orig_uuid_exist ] ; then
                echo "File should not be saved in storage1. storage_dir_size1 = $storage_dir_size1"
                echo "$copy_uuid and $orig_uuid uuids should be exist in storage2."
                kill_metax $p1 $p2
                exit;
        fi

        curl http://localhost:7091/db/get/?id=$uuid --output $get_file2
        diff=$(diff $save_file $get_file2 2>&1)
        if [ -n "$diff" ]; then
                echo "GET request failed after copy."
                kill_metax $p1 $p2
                exit;
        fi

        echo $PASS > $final_test_result
        kill_metax $p1 $p2
}

main $@

