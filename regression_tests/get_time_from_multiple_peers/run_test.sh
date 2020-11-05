#!/bin/bash

# Test case flow: 
# - Runs one instance of metax without peers and saves a data
# - Runs an instance of metax on the same storage and connects to two peers
# - Logins to peers and gets the file three times with http://localhost... on
#   each peer simultaneously.
# - Checked the time of each file get to not exceed 0.5s

if [ "yes" == "$use_valgrind" ]; then
        echo $SKIP > $final_test_result
        exit;
fi

get_file=$test_dir/get_file

# $1 password
# $2 peer username@ip
# $3 metax get url
get_from_peer()
{
        sshpass -p $1 ssh -oStrictHostKeyChecking=no $2  "cd /tmp/; /usr/bin/time -f %e --quiet -o get_time --append curl $3 -k 2>/dev/null 1>> get_result" &
        sshpass -p $1 ssh -oStrictHostKeyChecking=no $2  "cd /tmp/; /usr/bin/time -f %e --quiet -o get_time --append curl $3 -k 2>/dev/null 1>> get_result" &
        sshpass -p $1 ssh -oStrictHostKeyChecking=no $2  "cd /tmp/; /usr/bin/time -f %e --quiet -o get_time --append curl $3 -k 2>/dev/null 1>> get_result"
        ip=$(echo $2 | cut -d@ -f2)
        sshpass -p $1 scp -oStrictHostKeyChecking=no $2:/tmp/get_result $test_dir/peer${ip}_result
        sshpass -p $1 scp -oStrictHostKeyChecking=no $2:/tmp/get_time $test_dir/peer${ip}_time
        sshpass -p $1 ssh -oStrictHostKeyChecking=no $2 "rm /tmp/{get_result,get_time}"
}

# $1 password
# $2 peer username@ip
# $3 peer's storage
# $4 user json uuid
delete_user_json_from_peer()
{
        sshpass -p $1 ssh -oStrictHostKeyChecking=no $2 "rm $3/$4" 
}

source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0




main()
{
        $EXECUTE -f $tst_src_dir/config_wo_peers.xml &
        pid=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config_wo_peers.xml)
        echo -n $pid > $test_dir/processes_ids
        save_content="test_data"
        uuid=$(curl --data "$save_content" http://localhost:7081/db/save/node?enc=0)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        echo "uuid=$uuid"
        kill_metax $pid

        $EXECUTE -f $tst_src_dir/config.xml &
        pid=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7080 does not connect to peers"
                kill_metax $pid
                exit;
        fi
        echo -n $pid > $test_dir/processes_ids

        peer1=pi@172.17.7.65
        pass1='bananapi'
        url1="http://localhost:7081/db/get?id=$uuid\&cache=0"
        get_from_peer $pass1 $peer1 $url1 &

        peer2=metax@172.17.10.14 # gyumri.leviathan.am
        pass2='metax'
        url2="https://localhost:7081/db/get?id=$uuid\&cache=0"
        get_from_peer $pass2 $peer2 $url2
        sleep $TIME

        st=0
        ip=$(echo $peer1 | cut -d@ -f2)
        d1=$(diff $test_dir/peer${ip}_result $tst_src_dir/out.gold 2>&1)
        if [[ ! -z $d1 ]]; then st=1; fi
        while read t
        do
                if (( $(bc <<<"$t > 0.5") ))
                then
                        st=1
                        echo "network time exceeded boundaries: $t"
                fi
        done <$test_dir/peer${ip}_time

        ip=$(echo $peer2 | cut -d@ -f2)
        d2=$(diff $test_dir/peer${ip}_result $tst_src_dir/out.gold 2>&1)
        if [[ ! -z $d2 ]]; then st=1; fi
        while read t
        do
                echo "t=$t"
                if (( $(bc <<<"$t > 0.5") ))
                then
                        st=1
                        echo "network time exceeded boundaries: $t"
                fi
        done <$test_dir/peer${ip}_time
        if [[ "$st" -eq 0 ]]; then
                echo $PASS > $final_test_result
        fi

        # delete previously saved all uuids.
        expected='{"deleted":"'$uuid'"}'
        res=$(curl http://localhost:7081/db/delete?id=$uuid)
        echo "Delete request response is $res"
        if [ "$expected" != "$res" ]; then
                echo $FAIL > $final_test_result
        fi

        # delete user json uuid from peers
        user_json_uuid=$(decrypt_user_json_info $test_dir/keys | jq '.uuid' | sed -e 's/^"//' -e 's/"$//')
        peer1_storage=".leviathan/storage_for_test_environment"
        peer2_storage=".leviathan/storage/storage_for_test_environment"
        delete_user_json_from_peer $pass1 $peer1 $peer1_storage $user_json_uuid
        delete_user_json_from_peer $pass2 $peer2 $peer2_storage $user_json_uuid

        kill_metax $pid
}

main $@
