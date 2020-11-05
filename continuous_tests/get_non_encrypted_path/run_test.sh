#!/bin/bash

# Test case flow: 
# - Runs and connects two instances of metax with config1.xml and config2.xml
#   configurations (the peers have different device/user keys).
# - Sends "path" save request to first metax (config1.xml) for some random
#   generated file (should be saved as non-encrypted).
# - Sends get request to the second running metax (config2.xml) for uuid, which
#   has been saved in the first metax.
# - Checks the compliance of received/saved contents.

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
        echo -n $p2 $p1 > $test_dir/processes_ids
        #dd-"data dublicator". Dublicates random data in save_file
        #according to the given options.
        dd if=/dev/urandom of=$save_file bs=1000000 count=1
        uuid=$(curl --data "$save_file" http://localhost:7081/db/save/path?enc=0)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        curl http://localhost:7081/db/get/?id=$uuid --output $get_file
        d=$(diff $save_file $get_file) 
        if [[ -z $d ]]; then
            echo $PASS > $final_test_result
        fi          
        kill_metax $p1 $p2
}

main $@

# vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:
