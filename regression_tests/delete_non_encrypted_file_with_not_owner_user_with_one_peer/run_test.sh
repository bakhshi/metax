#!/bin/bash

# Test case flow: 
# - Runs and connects two instances of metax ( metax1 -> metax2 ).
# - Sends "data" save request for random generated file (enc = 0) to the metax1.
# - Sends delete request to the not owner user:metax2
# - Sends get request for received uuid to the running metaxes.
# - Checks that delete from not owner user succeeded.

save_file=$test_dir/save_file
get_file=$test_dir/get_file


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0





main()
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
        get_user_json_uuids 2
        str=$(echo ${uj_uuids[@]} | sed 's/ /*\\|/g')

        dd if=/dev/urandom of=$save_file bs=1000000 count=1
        uuid=$(curl --form "fileupload=@$save_file" http://localhost:7081/db/save/data?enc=0)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        # sleep for backup save
        sleep $TIME
        ls $test_dir/storage1/????????-* | grep -v "$str" &> /dev/null
        rc1=$?;
        ls $test_dir/storage2/????????-* | grep -v "$str" &> /dev/null
        rc2=$?;
        if [ "$rc1" != "0" ] || [ "$rc2" != "0" ]; then
                echo $FAIL > $final_test_result
                echo "Storage should not be empty: $rc1, $rc2"
                kill_metax $p1 $p2
                exit;
        fi
        curl http://localhost:7081/db/get/?id=$uuid --output $get_file
        d1=$(diff $save_file $get_file 2>&1)
        if [[ -n $d1 ]] ; then
                echo "Could not get saved data"
                kill_metax $p2 $p1
                exit;
        fi

        expected='{"deleted":"'$uuid'"}'
        res=$(curl http://localhost:7091/db/delete/?id=$uuid)
        if [ "$expected" != "$res" ] ; then
                echo "Failed to delete from not owner user. Response is $res"
                kill_metax $p2 $p1
                exit;
        fi
        sleep $TIME
        ls $test_dir/storage1/????????-* | grep -v "$str" &> /dev/null
        rc1=$?;
        ls $test_dir/storage2/????????-* | grep -v "$str" &> /dev/null
        rc2=$?;
        if [ "$rc1" == "0" ] || [ "$rc2" == "0" ]; then
                echo $FAIL > $final_test_result
                echo "Storage should be empty $rc1, $rc2"
                kill_metax $p1 $p2
                exit;
        fi

        expected='{"error":"Getting file failed: uuid='$uuid'."}'
        get=$(curl http://localhost:7081/db/get?id=$uuid)
        if [ "$expected" != "$get" ]; then
                echo $FAIL > $final_test_result
                echo "The get of deleted file should be failed. Response is $res"
                kill_metax $p1 $p2
                exit;
        fi

        echo $PASS > $final_test_result
        kill_metax $p2 $p1
}

main $@
