#!/bin/bash

# Test case flow:
# - Runs and connects three instances of metax (metax2 -> metax1, metax2 -> metax3 (storage_class=0)).
# - Generates .mp4 random file.
# - Sends "data" save request for generated file with "Metax-Content-Type video/mp4" and enc=0 parameters.
# - Send get requests to get mp4 from metax1 and metax3.
# - Check that get sata request is succeeded or not.

source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0
save_file=$test_dir/save_file.mp4
get_file1=$test_dir/get_file1.mp4
get_file3=$test_dir/get_file3.mp4




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

        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7080 does not connect to peers"
                kill_metax $p3 $p1
                exit;
        fi

        $EXECUTE -f $tst_src_dir/config2.xml &
        p2=$!
        is_peer_connected=$(peers_connect 7091 $tst_src_dir/config2.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7090 does not connect to peer"
                kill_metax $p3 $p2 $p1
                exit;
        fi
        echo -n $p3 $p2 $p1 > $test_dir/processes_ids
}




main()
{
        start_metaxes
        #Generated ~5MB file
        dd if=/dev/urandom of=$save_file bs=100000 count=4

        uuid=$(curl --form "fileupload=@$save_file" -H "Metax-Content-Type: video/mp4" http://localhost:7091/db/save/data?enc=0)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        curl http://localhost:7081/db/get/?id=$uuid --output $get_file1 &
        get_pid=$!

        curl http://localhost:7101/db/get/?id=$uuid --output $get_file3

        wait $get_pid
        diff1=$(diff $save_file $get_file1)
        diff2=$(diff $save_file $get_file3)
        if [ -n "$diff" ] || [ -n "$diff1" ];then
                echo "Failed to get .mp4 file from peer1 or peer3"
                kill_metax $p1 $p2 $p3
                exit;
        fi

        echo $PASS > $final_test_result
        kill_metax $p1 $p2 $p3
}

main $@
