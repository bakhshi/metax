#!/bin/bash

# Test case flow:
# - Runs and connects two instances of metax (peer1->peer2).
# - Sends "save" request for getting a valid uuid.
# - Tries to find "master_uuid" in the shared object.
# - Removes one uuid, which is not a shared object or master uuid, from the
#   storages of both peers.
# - Sends "get" request to get an object, which chunk is missing.
# - Checks the compliance of received/expected messages.

rm -rf $test_dir/storage1
rm -rf $test_dir/storage2
save_file=$test_dir/save_file
get_file=$test_dir/get_file

source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0


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
        # Generate ~8MB random file
        dd if=/dev/urandom of=$save_file bs=8000000 count=1

        start_metaxes
        uj_uuid1=$(decrypt_user_json_info $test_dir/keys1 | jq '.uuid' | sed -e 's/^"//' -e 's/"$//')
        uj_uuid2=$(decrypt_user_json_info $test_dir/keys2 | jq '.uuid' | sed -e 's/^"//' -e 's/"$//')

        save=$(curl --data "$save_file" http://localhost:7081/db/save/path?local=0)
        uuid=$(echo $save | cut -d'"' -f 4)
        curl http://localhost:7081/db/get?id=$uuid\&cache=0 --output $get_file
        d=$(diff $save_file $get_file 2>&1)
        if [[ -n $d ]]; then
                echo "Failed to get previously saved data."
                kill_metax $p1 $p2
                exit
        fi

        echo "Saved uuid is $uuid."
        kill_metax $p1 $p2

        # remove chunk from metax2.
        ls_storage=$(ls $test_dir/storage2/????????-*)
        to_del=$(ls $test_dir/storage2/????????-* | grep -v $uuid | grep -v "$uj_uuid1\|$uj_uuid1.tmp" | grep -v "$uj_uuid2\|$uj_uuid2.tmp" | head -n 1)
        rm -v $to_del
        chunk_uuid="$(basename $to_del)"

        start_metaxes

        expected='{"error":"Getting file failed: uuid='$uuid'. There is a missing chunk in the requested object: chunk_uuid = '$chunk_uuid'"}'
        res=$(curl -D $test_dir/h http://localhost:7081/db/get?id=$uuid\&cache=0)
        echo $res
        if [ "$res" != "$expected" ]; then
                echo "Invalid response for data with missing chunk. Res = $res, Expected = $expected"
                kill_metax $p1 $p2
                exit
        fi
        http_code=$(cat $test_dir/h | grep HTTP/1.1 | awk {'print $2'})
        echo "HTTP code of GET request response is $http_code"
        if [[ 404 != $http_code ]]; then
                echo "HTTP code of GET request response is $http_code"
                kill_metax $p1 $p2
                exit;
        fi

        echo $PASS > $final_test_result
        kill_metax $p1 $p2
}

main $@

# vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:
