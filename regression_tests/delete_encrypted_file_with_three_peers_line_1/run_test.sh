#!/bin/bash

# Test case flow: 
# - Runs and connects four instances of metax (peer1->peer2->peer3->peer4).
# - Sends "data" save request for random generated file (should be saved as non-encrypted)
#   to the metax, running with config1.xml.
# - Sends get request for received uuid to the running metaxes.
# - Checks the compliance of received/saved contents.

save_file=$test_dir/save_file
get_file=$test_dir/get_file


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0





start_metaxes()
{ 
        $EXECUTE -f $tst_src_dir/config4.xml &
        p4=$!
        is_peer_connected=$(peers_connect 7111 $tst_src_dir/config4.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7111 does not connect to peer"
                kill_metax $p4
                exit;
        fi
        $EXECUTE -f $tst_src_dir/config3.xml &
        p3=$!
        is_peer_connected=$(peers_connect 7101 $tst_src_dir/config3.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7101 does not connect to peer"
                kill_metax $p4 $p3
                exit;
        fi
        $EXECUTE -f $tst_src_dir/config2.xml &
        p2=$!
        is_peer_connected=$(peers_connect 7091 $tst_src_dir/config2.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7090 does not connect to peer"
                kill_metax $p2 $p3 $p4
                exit;
        fi

        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7080 does not connect to peers"
                kill_metax $p4 $p3 $p2 $p1
                exit;
        fi
        echo -n $p4 $p3 $p2 $p1 > $test_dir/processes_ids
}



main()
{
        start_metaxes
        get_user_json_uuids 4
        str=$(echo ${uj_uuids[@]} | sed 's/ /*\\|/g')

        dd if=/dev/urandom of=$save_file bs=1000000 count=1
        uuid=$(curl --form "fileupload=@$save_file" http://localhost:7081/db/save/data)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        sleep $TIME
        ls $test_dir/storage1/????????-* | grep -v "$str" &> /dev/null
        rc1=$?;
        ls $test_dir/storage2/????????-* | grep -v "$str" &> /dev/null
        rc2=$?;
        ls $test_dir/storage3/????????-* | grep -v "$str" &> /dev/null
        rc3=$?;
        ls $test_dir/storage4/????????-* | grep -v "$str" &> /dev/null
        rc4=$?;
        echo "$rc1 $rc2 $rc3 $rc4"
        if [ "$rc1" != "0" ] || [ "$rc2" != "0" ] || [ "$rc3"  != "0" ] || [ "$rc4" != "0" ]; then
                echo $FAIL > $final_test_result
                echo "Storage should not be empty"
                kill_metax $p1 $p2 $p3 $p4
                exit;
        fi
        curl http://localhost:7081/db/get/?id=$uuid --output $get_file
        d1=$(diff $save_file $get_file 2>&1)
        if [[ -n $d1 ]] ; then
                echo "Could not get saved data"
                kill_metax $p4 $p3 $p2 $p1
                exit;
        fi

        expected='{"deleted":"'$uuid'"}'
        res=$(curl http://localhost:7081/db/delete/?id=$uuid)
        if [ "$expected" != "$res" ] ; then
                echo "Failed to delete. Response is $res"
                kill_metax $p4 $p3 $p2 $p1
                exit;
        fi
        sleep $TIME
        ls $test_dir/storage1/????????-* | grep -v "$str" &> /dev/null
        rc1=$?;
        ls $test_dir/storage2/????????-* | grep -v "$str" &> /dev/null
        rc2=$?;
        ls $test_dir/storage3/????????-* | grep -v "$str" &> /dev/null
        rc3=$?;
        ls $test_dir/storage4/????????-* | grep -v "$str" &> /dev/null
        rc4=$?;
        echo "$rc1 $rc2 $rc3 $rc4"
        if [ "$rc1" == "0" ] || [ "$rc2" == "0" ] || [ "$rc3"  == "0" ] || [ "$rc4" == "0" ]; then
                echo $FAIL > $final_test_result
                echo "Storage should be empty"
                kill_metax $p1 $p2 $p3 $p4
                exit;
        fi

        expected='{"error":"Getting file failed: uuid='$uuid'."}'
        get=$(curl http://localhost:7081/db/get?id=$uuid)
        if [ "$expected" != "$get" ]; then
                echo $FAIL > $final_test_result
                echo "The get of deleted file should be failed. Response is $res"
                kill_metax $p1 $p2 $p3 $p4
                exit;
        fi

        echo $PASS > $final_test_result
        kill_metax $p4 $p3 $p2 $p1
}

main $@
