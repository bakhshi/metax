#!/bin/bash

#TODO-This test also is PASS, when save process fails, because it counts only saving time
#     and not checks if save succeeded.(Bug #3428)

# Test case flow
# - Runs four instances of metax with line connection.
# - Sends 10 "data" save requests in localhost for random generated file
#   and counts the average request time.
# - "Save" request time shouldn't exceed 0.7 seconds

if [ "yes" == "$use_valgrind" ]; then
        echo $SKIP > $final_test_result
        exit;
fi

save_file=$test_dir/save_file
function get_SAVE_request_time()
{
        time (curl --data "$save_file" http://localhost:7081/db/save/path --output $test_dir/save_result) 2> $test_dir/time_log
        save_res=$(cat $test_dir/save_result)
        uuid=$(echo $save_res | jq '.uuid')
        if [ -z $save_res ] || [ "null" == "$uuid" ]; then
                echo "Failed to save file"
                return;
        fi
        a=$(cat $test_dir/time_log | grep real | awk '{print $2}')
        t1=$(echo $a | cut -d'm' -f1)
        t2=$(echo $a | cut -d'm' -f2 | cut -d'.' -f1)
        t3=$(echo $a | cut -d'm' -f2 | cut -d'.' -f2 | head -c 3)
        t1=$(bc <<< "scale=3; 60 * $t1")
        t=$(bc <<< "scale=3; $t1 + $t2")
        t=$(bc <<< "scale=3; $t * 1000")
        t=$(bc <<< "scale=3; $t + $t3")
        echo $t;
}



source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0




start_metaxes()
{
        $EXECUTE -f $tst_src_dir/config4.xml &
        p4=$!
        is_peer_connected=$(peers_connect 7111 $tst_src_dir/config4.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7111 does not connect to peer"
                kill_metax $p4
                exit;
        fi
        $EXECUTE -f $tst_src_dir/config3.xml &
        p3=$!
        is_peer_connected=$(peers_connect 7101 $tst_src_dir/config3.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7101 does not connect to peer"
                kill_metax $p4 $p3
                exit;
        fi
        $EXECUTE -f $tst_src_dir/config2.xml &
        p2=$!
        is_peer_connected=$(peers_connect 7091 $tst_src_dir/config2.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7090 does not connect to peer"
                kill_metax $p4 $p3 $p2
                exit;
        fi

        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7080 does not connect to peers"
                kill_metax $p4 $p3 $p2 $p1
                exit;
        fi
        echo -n $p4 $p3 $p2 $p1 > $test_dir/processes_ids
}

main()
{
        start_metaxes
        dd if=/dev/urandom of=$save_file bs=10000000 count=1

        a=0
        for i in {1..10}
        do
                b=$(get_SAVE_request_time)
                echo "a$i=$b"
                if [[ "$b" =~ "Failed to save file" ]];then
                        echo "Failed to save file. get_SAVE-request_time=$b"
                        kill_metax $p4 $p3 $p2 $p1;
                        exit;
                fi
                a=$(($a + $b))
        done

        echo "a=$a"
        a=$(($a / 10))
        a=$(bc -l <<< "scale=3; $a / 1000")
        r=$(bc -l <<< "$a < 0.700")
        if [ 1 == "$r" ]; then
                echo $PASS > $final_test_result;
        else
                echo $a is greater than or equal to 0.700
        fi
        echo "Local save speed is $a seconds"
        kill_metax $p1 $p2 $p3 $p4
}

main $@
