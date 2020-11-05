#!/bin/bash

# Test case flow:
# - Runs and connects two instances of metax (peer1->peer2).
# - Sends "save" request for getting a valid uuid.
# - Tries to find "master_uuid" in the shared object.
# - Removes one uuid, which is not a shared object or master uuid, from the
#   peer1's storage.
# - Sends "get" request to get an object, from which one of the chunks is missing.
# - Checks the compliance of saved/get objects as the missing chunk should be
#   taken from its peer.

rm -rf $test_dir/storage1
rm -rf $test_dir/storage2


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0
save_file=$test_dir/save_file
get_file=$test_dir/get_file

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
                kill_metax $p2 $p1
                exit;
        fi
        echo -n $p1 $p2 > $test_dir/processes_ids
}

main()
{
        start_metaxes

        uj_uuid1=$(decrypt_user_json_info $test_dir/keys1 | jq '.uuid' | sed -e 's/^"//' -e 's/"$//')
        uj_uuid2=$(decrypt_user_json_info $test_dir/keys2 | jq '.uuid' | sed -e 's/^"//' -e 's/"$//')

        dd if=/dev/urandom of=$save_file bs=8000000 count=1
        save=$(curl --form "fileupload=@$save_file" http://localhost:7081/db/save/data)
        master_uuid=$(echo $save | cut -d'"' -f 4)
        echo master_uuid = $master_uuid
        sleep $TIME

        to_del=$(ls $test_dir/storage1/????????-* | grep -v $master_uuid | grep -v "$uj_uuid1\|$uj_uuid1.tmp" | grep -v "$uj_uuid2\|$uj_uuid2.tmp" | head -n 1)
        echo "to_del=$to_del"

        rm -v $to_del
        if [ "0" != "$?" ];then
                echo "Failed to remove uuid."
                kill_metax $p1 $p2
                exit;
        fi

        curl http://localhost:7081/db/get/?id=$master_uuid --output $get_file
        d=$(diff $save_file $get_file 2>&1)
        if [ -z $d ]; then
                echo $PASS > $final_test_result
                kill_metax $p1 $p2
                exit;
        fi
        echo diff = $d
        kill_metax $p1 $p2
}

main $@

# vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:
