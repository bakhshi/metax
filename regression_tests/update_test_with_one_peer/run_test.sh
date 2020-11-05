#!/bin/bash

# Test case flow:
# - Runs and connects two instances of metax (peer2->peer1).
# - Sends "save" request for getting a valid uuid.
# - Sends "update" requests to the running metaxes three time.
# - Checks the compliance of "save"/"update" request's messages.


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0




main()
{
        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7080 does not connect to peer"
                kill_metax $p1
                exit;
        fi
        $EXECUTE -f $tst_src_dir/config2.xml &
        p2=$!
        is_peer_connected=$(peers_connect 7091 $tst_src_dir/config2.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7090 does not connect to peer"
                kill_metax $p2 $p1
                exit;
        fi
        echo -n $p1 $p2 > $test_dir/processes_ids
        uj_uuid1=$(decrypt_user_json_info $test_dir/keys1 | jq '.uuid' | sed -e 's/^"//' -e 's/"$//')
        uj_uuid2=$(decrypt_user_json_info $test_dir/keys2 | jq '.uuid' | sed -e 's/^"//' -e 's/"$//')
        diff_options="--exclude=cache --exclude=storage.json --exclude=$uj_uuid1* --exclude=$uj_uuid2*"
        save=$(curl --data "save content" http://localhost:7081/db/save/node)
        uuid=$(echo $save | cut -d'"' -f 4)

        for ((i=1; i<=3; ++i))
        do
                update_data="updated content $i time"
                update=$(curl -m 10 --data "$update_data" http://localhost:7081/db/save/node?id=$uuid)
                echo $update
                if [ "$save" != "$update" ]; then
                        echo "Failed updating from peer1 $i time"
                        kill_metax $p1 $p2
                        exit
                fi

                res=$(curl -m 10 http://localhost:7081/db/get?id=$uuid)
                if [ "$res" != "$update_data" ]; then
                        echo "Failed to get updated content $i time"
                        kill_metax $p1 $p2
                        exit
                fi
        done

        sleep $TIME
        d=$(diff -r $test_dir/storage1/ $test_dir/storage2/ $diff_options)
        if [[ -n $d ]] ; then
                echo "Storage directories are not the same"
                kill_metax $p1 $p2
                exit;
        fi

        echo $PASS > $final_test_result
        kill_metax $p1 $p2
}

main $@

# vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:
