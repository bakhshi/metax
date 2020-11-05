#!/bin/bash

# Test case flow:
# - Runs and connects two instances of metax ( metax1 -> metax2 ).
# - Sends "data" save request for random generated file to the metax1 with no local.
# - Sends delete request to the metax1.
# - Checks that delete from metax1 succeeded.

save_file=$test_dir/save_file


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0



check_storage_content()
{
        while read l; do
                if  [[ ! "${uj_uuids[@]}" =~ "${l}" ]]; then
                        return 1;
                fi
        done < $1
        return 0;
}




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
                kill_metax $p2 $p1
                exit;
        fi
        echo -n $p2 $p1 > $test_dir/processes_ids
        get_user_json_uuids 2
        str=$(echo ${uj_uuids[@]} | sed 's/ /*\\|/g')

        dd if=/dev/urandom of=$save_file bs=1000000 count=1
        uuid=$(curl --form "fileupload=@$save_file" "http://localhost:7081/db/save/data?enc=0&local=0")
        uuid=$(echo $uuid | cut -d'"' -f 4)
        echo "uuid = $uuid"
        # sleep for backup save
        sleep $TIME
        ls $test_dir/storage1/????????-* | grep -v "$str" | rev | cut -d '/' -f1 | rev > $test_dir/uuids

        check_storage_content $test_dir/uuids
        if [ "0" != "$?" ];then
                echo "Storage should contain only uuids of user json."
                kill_metax $p1 $p2
                exit;
        fi

        ls $test_dir/storage2/????????-* &> /dev/null
        rc2=$?;
        if [ "$rc2" != "0" ]; then
                echo $FAIL > $final_test_result
                echo "storage2 should not be empty: $rc2"
                rm $save_file
                kill_metax $p1 $p2
                exit;
        fi
        expected='{"deleted":"'$uuid'"}'
        res=$(curl http://localhost:7081/db/delete/?id=$uuid)
        if [ "$expected" != "$res" ] ; then
                echo "Failed to delete. Response is $res"
                rm $save_file
                kill_metax $p2 $p1
                exit;
        fi

        expected='{"error":"Getting file failed: uuid='$uuid'."}'
        get=$(curl http://localhost:7081/db/get?id=$uuid)
        if [ "$expected" != "$get" ]; then
                echo "The get of deleted file should be failed. Response is $get"
                kill_metax $p1 $p2
                rm $save_file
                exit;
        fi

        echo $PASS > $final_test_result
        rm $save_file
        kill_metax $p2 $p1
}

main $@
