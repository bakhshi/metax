#!/bin/bash

# Test case flow: 
# - Runs an instances of metax.
# - Sends "data" save request for a random generated file (should be saved as non-encrypted).
# - Waits about 5 seconds for backup.
# - Counts the average get request time for 5 requests.
# - Counts the average scp time for 5 requests.
# - Compares "get" and "scp" times."get" time shouldn't exceed "scp" time.

if [ "yes" == "$use_valgrind" ]; then
        echo $SKIP > $final_test_result
        exit;
fi

save_file=$test_dir/save_file
get_file=$test_dir/get_file_
server="172.17.7.65"
username="pi"
pass="bananapi"

function get_GET_request_time()
{
        get_respose=$( { time curl http://localhost:7081/db/get/?id=$uuid\&cache=0 -o  $get_file$1; } 2>$test_dir/time_log_metax_$1)
        a=$(cat $test_dir/time_log_metax_$1 | grep real | awk '{print $2}')
        t1=$(echo $a | cut -d'm' -f1)
        t2=$(echo $a | cut -d'm' -f2 | cut -d's' -f1)
        #t2=1.105
        t1=$(bc <<< "scale=3; 60*$t1")
        t=$(bc <<< "scale=3; $t1 + $t2")
        echo $t;
}

function get_scp_time()
{
        r=$( { time sshpass -p $pass scp -o StrictHostKeyChecking=no $save_file $username@$server:/home/$username ; } 2>$test_dir/time_log)
        a=$(cat $test_dir/time_log | grep real | awk '{print $2}')
        t1=$(echo $a | cut -d'm' -f1)
        t2=$(echo $a | cut -d'm' -f2 | cut -d's' -f1)
        t1=$(bc <<< "scale=3; 60*$t1")
        t=$(bc <<< "scale=3; $t1 + $t2")
        echo $t;
}


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0





main()
{
        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7080 does not connect to peers"
                kill_metax $p1
                exit;
        fi
        echo -n $p1 > $test_dir/processes_ids
        ping -c 10 $server
        if [[ "0" != "$?" ]] ; then
                echo "$server server in offline"
                echo $FAIL > $final_test_result
                kill_metax $p1
                exit
        fi
        dd if=/dev/urandom of=$save_file bs=16000000 count=1
        uuid=$(curl --form "fileupload=@$save_file" "http://localhost:7081/db/save/data?enc=0&local=0")
        uuid=$(echo $uuid | cut -d'"' -f 4)
        echo "uuid=$uuid"
        sleep $TIME
        t_metax=$(bc <<< "scale=3; ($(get_GET_request_time 1) + $(get_GET_request_time 2) + $(get_GET_request_time 3) + $(get_GET_request_time 4) + $(get_GET_request_time 5))/5")
        t_scp=$(bc <<< "scale=3; ($(get_scp_time) + $(get_scp_time) + $(get_scp_time) + $(get_scp_time) + $(get_scp_time))/5")

        echo "Metax get request time = $t_metax" >> $test_dir/metax_get_time.txt
        echo "Scp time = $t_scp" >> $test_dir/scp_time.txt
        t_scp=$(bc <<< "scale=3; 2*$t_scp")
        echo $t_metax >> $test_dir/test_output
        echo $t_scp >> $test_dir/test_output
        if (( $(bc <<<"$t_metax < $t_scp") )) ; then
                echo $PASS > $final_test_result
        fi
	curl http://localhost:7081/db/delete?id=$uuid
        
	kill_metax $p1
}

main $@
