#!/bin/bash

# Test case flow:
# - Runs and connects three instances of metax (peer2->peer1, peer3->peer1).
# - Sends "save" request for getting a valid uuid.
# - Sends "update" requests to the running metaxes three time.
# - Checks the compliance of "save"/"update" request's messages.


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0




start_metaxes()
{
        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7080 does not connect to peers"
                kill_metax $p1
                exit;
        fi

        $EXECUTE -f $tst_src_dir/config2.xml &
        p2=$!
        is_peer_connected=$(peers_connect 7091 $tst_src_dir/config2.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7090 does not connect to peer"
                kill_metax $p1 $p2
                exit;
        fi

        $EXECUTE -f $tst_src_dir/config3.xml &
        p3=$!
        is_peer_connected=$(peers_connect 7101 $tst_src_dir/config3.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7101 does not connect to peer"
                kill_metax $p3 $p2 $p1
                exit;
        fi
        echo -n $p3 $p2 $p1 > $test_dir/processes_ids
}



main()
{
        start_metaxes
        get_user_json_uuids 3

        diff_options="--exclude=cache --exclude=storage.json"
        for i in ${!uj_uuids[@]}; do
                diff_options+=" --exclude=${uj_uuids[$i]}*"
        done

        save=$(curl --data "save content" http://localhost:7081/db/save/node)
        uuid=$(echo $save | cut -d'"' -f 4)
	sleep $TIME

        for ((i=1; i<=3; ++i))
        do
                update_data="updated content $i time"
                update=$(curl -m 10 --data "$update_data" http://localhost:7081/db/save/node?id=$uuid)
                echo $update
                if [ "$save" != "$update" ]; then
                        echo "Failed updating from peer1"
                        echo $FAIL > $final_test_result
			kill_metax $p1 $p2 $p3
                        exit
                fi

                res=$(curl -m 10 http://localhost:7081/db/get?id=$uuid)
                if [ "$res" != "$update_data" ]; then
                        echo "Fail to get updated data $i time"
                        echo $FAIL > $final_test_result
			kill_metax $p1 $p2 $p3
                        exit
                fi
        done

        sleep $TIME
        d1=$(diff -r $test_dir/storage1/ $test_dir/storage2/ $diff_options)
        d2=$(diff -r $test_dir/storage1/ $test_dir/storage3/ $diff_options)
        if [[ -n $d1 || -n $d2 ]] ; then
                echo "Storage directories are not the same"
                echo $FAIL > $final_test_result
                kill_metax $p1 $p2 $p3
                exit;
        fi

        echo $PASS > $final_test_result
        kill_metax $p1 $p2 $p3
}

main $@

# vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:
