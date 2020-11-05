#!/bin/bash

# Test case flow:
# - Runs and connects two instances of metax.
# - Sends "node" save request for some content with enc=0 parameter.
# - Waits about 10 second for backup then kills all metaxes.
# - Runs metax with config2.xml configuration.
# - Sends get request for received uuid to running metax.
# - Checks the compliance of received/saved contents.



source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0


clean_storage_content()
{
        uj_uuid1=("$(decrypt_user_json_info $tst_src_dir/keys1 | jq '.uuid' | sed -e 's/^"//' -e 's/"$//')")
        uj_uuid2=("$(decrypt_user_json_info $test_dir/keys2 | jq '.uuid' | sed -e 's/^"//' -e 's/"$//')")
        echo "remove uuids (except user jsons) from $1  directory"
        str=" -o -name $uj_uuid1 -o -name $uj_uuid2"
        find $1 -type f ! \( -name storage.json $str \) -delete
}

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
                kill_metax $p1 $p2
                exit;
        fi
        echo -n $p2 $p1 > $test_dir/processes_ids
}


main()
{
        cp -r $tst_src_dir/storage1 $test_dir/storage1
        cp -r $tst_src_dir/storage2 $test_dir/storage2
        start_metaxes
        save_content="save_content"
        uuid="de1ab13f-0f57-4c02-95e6-baae1cae6d7f"
        get_content=$(curl http://localhost:7081/db/get/?id=$uuid)
        if [ "$save_content" != "$get_content" ]; then
                echo "GET request failed. get_content=$get_content"
                kill_metax $p1 $p2
                exit;
        fi

        kill_metax $p1 $p2
        clean_storage_content $test_dir/storage1
        start_metaxes

        get_content=$(curl http://localhost:7081/db/get/?id=$uuid)
        if [ "$save_content" != "$get_content" ]; then
                echo "GET request failed after removing storage1. get_content=$get_content"
                kill_metax $p1 $p2
                exit;
        fi

        echo $PASS > $final_test_result
        kill_metax $p1 $p2
}

main $@
