#!/bin/bash

# Test case flow: 
# - Runs and connects 3 instance of metax with the following scheme:
#       - 2 connects to 1
#       - 3 connects to 1
# - Generates random file.
# - Sends "data" save request for generated file with enc=0 parameter to peer 1
# - Waits about 15 second for backup.
# - Checks for existence of bakcup in 2 and 3 peers


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
                kill_metax $p2 $p1
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
        cd $test_dir
        start_metaxes
        get_user_json_uuids 3
        str=$(echo ${uj_uuids[@]} | sed 's/ /*\\|/g')

        dd if=/dev/urandom of=save_file bs=16777216 count=1
        curl --form "fileupload=@save_file" http://localhost:7081/db/save/data?enc=0
        sleep $TIME
	storage=$(ls storage1/????????-* | grep -v "$str" | expr $(wc -l))
	bckp1=$(ls storage2/????????-* | grep -v "$str" | expr $(wc -l))
	bckp2=$(ls storage3/????????-* | grep -v "$str" | expr $(wc -l))
        if [[ $bckp1 -eq $storage ]] && [[ $bckp2 -eq $storage ]]; then 
                echo $PASS > $final_test_result
        fi
        kill_metax $p1 $p2 $p3
}
 
main $@
