#!/bin/bash

# Test case flow:
# - Runs and connects two instance of metax.
# - Generates random file.
# - Sends "data" save request for generated file (default it is saved as encrypted).
# - Waits about 5 second for backup then kills all metaxes.
# - Runs metax with config2.xml configuration.
# - Sends get request for received uuid to running metax.
# - Checks the compliance of received/saved files.

save_file=$test_dir/save_file
get_file=$test_dir/get_file


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
        start_metaxes
        get_user_json_uuids 2
        str=$(echo ${uj_uuids[@]} | sed 's/ /*\\|/g')
        dd if=/dev/urandom of=$save_file bs=16000000 count=1
        uuid=$(curl --form "fileupload=@$save_file" http://localhost:7081/db/save/data?local=0)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        sleep $TIME
        kill_metax $p1 $p2
        ls $test_dir/storage1/????????-* | grep -v "$str" | rev | cut -d '/' -f1 | rev > $test_dir/uuids

        check_storage_content $test_dir/uuids
        if [ "0" != "$?" ];then
                echo "Storage should contain only uuids of user json."
                exit;
        fi

        start_metaxes
        curl http://localhost:7081/db/get/?id=$uuid --output $get_file
        d=$(diff $save_file $get_file 2>&1)
        if [[ -z $d ]]; then
                echo $PASS > $final_test_result
        fi
        kill_metax $p1 $p2
}

main $@
