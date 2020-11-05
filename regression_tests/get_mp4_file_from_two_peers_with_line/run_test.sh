#!/bin/bash

# Test case flow:
# - Runs and connects three instances of metax (metax1 (local=0) -> metax2 (storage_class=0) -> metax3).
# - Generates .mp4 random file.
# - Sends "data" save request for generated file with "Metax-Content-Type video/mp4" and enc=0 parameters.
# - Send get request for previously saved data to metax2.
# - Check that get sata request is succeeded or not.

source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0
save_file=$test_dir/save_file.mp4
get_file=$test_dir/get_file.mp4





start_metaxes()
{
        $EXECUTE -f $tst_src_dir/config3.xml &
        p3=$!
        is_peer_connected=$(peers_connect 7101 $tst_src_dir/config3.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7101 does not connect to peer"
                kill_metax $p3
                exit;
        fi
        $EXECUTE -f $tst_src_dir/config2.xml &
        p2=$!
        is_peer_connected=$(peers_connect 7091 $tst_src_dir/config2.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7090 does not connect to peer"
                kill_metax $p3 $p2
                exit;
        fi

        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7080 does not connect to peers"
                kill_metax $p3 $p2 $p1
                exit;
        fi
        echo -n $p3 $p2 $p1 > $test_dir/processes_ids
}



main()
{
        start_metaxes
        #Generated ~5MB file
        dd if=/dev/urandom of=$save_file bs=100000 count=5

        uuid=$(curl --form "fileupload=@$save_file" -H "Metax-Content-Type: video/mp4" http://localhost:7081/db/save/data?enc=0\&local=0)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        curl http://localhost:7091/db/get/?id=$uuid --output $get_file

        diff=$(diff $save_file $get_file)
        if [ -n "$diff" ];then
                echo "Failed to get .mp4 file from peer2."
                kill_metax $p1 $p2 $p3
                exit;
        fi

        echo $PASS > $final_test_result
        kill_metax $p1 $p2 $p3
}

main $@
