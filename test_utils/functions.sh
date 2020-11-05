#!/bin/bash

# kill metax by pids
# $1, $2 ... - pids of running metaxes
kill_metax()
{
        kill $@
        for i in "$@"
        do
                wait "$i"
                if [ "$EXIT_CODE_OK" != "$?" ]; then
                        echo "Metax crashed"
                        echo $CRASH > $final_test_result
                fi
        done
}


# Check that http server was started
# $1 - web_server_port
wait_for_init()
{
        curl --retry-connrefused --retry-delay 3 --retry 30 http://localhost:$1/config/get_user_public_key >> $test_dir/retry_$1 2>&1
        if [ "0" != "$?" ]; then
                echo "Could not connect to localhost:$1"
                return;
        fi
        echo 0
}

# Check that all peers were connected.
# $1 - web_server_port
# $2 - config.xml path
peers_connect()
{
        is_http_server_started=$(wait_for_init $1)
        if [ "0" != "$is_http_server_started" ];then
                echo 1
                return;
        fi

        # parse config.xml and get peers list
        sed -n -e '/^[[:space:]]*<peers>/,/^[[:space:]]*<\/peers>/p' $2 | grep "<peer>" > $test_dir/peers_list_$1
        sed $SEDIP 's/ //g;s/<peer>//g;s/<\/peer>//g' $test_dir/peers_list_$1
        while read l
        do
                is_peer_connected=0
                for i in {1..10}; do
                        (curl http://localhost:$1/config/get_online_peers > $test_dir/online_peers_$1) &> /dev/null
                        grep -q '"address":"'$l'"' $test_dir/online_peers_$1
                        if [ "0" == "$?" ];then
                                is_peer_connected=1
                                break;
                        fi
                        sleep 1
                done
                if [ "0" == "$is_peer_connected" ]; then
                        echo "Could not connect to $l peer" >> $test_dir/failed_peers_list
                        echo 1
                        return;
                fi
        done < $test_dir/peers_list_$1

        if [ -s $test_dir/peers_list_$1 ]; then
                cp $project_root/test_utils/ws_for_sync_finished.js $test_dir
                nodejs ws_for_sync_finished.js $1
                if [ "0" != "$?" ]; then
                        echo "sync_finished doesn't receive" >> $test_dir/sync_finished_error
                        echo 1
                        return;
                fi
        fi
        echo 0
}

# Check that peer was connected.
# $1 - web_server_port
# $2 - peer's ip:port
peer_connected()
{
        is_http_server_started=$(wait_for_init $1)
        if [ "0" != "$is_http_server_started" ]; then
                echo 1
                return;
        fi

        for i in {1..10}; do
                (curl http://localhost:$1/config/get_online_peers > $test_dir/peer_${2}) &> /dev/null
                grep -q '"address":"'$2'"' $test_dir/peer_${2}
                if [ "0" == "$?" ];then
                        echo 0
                        return;
                fi
                sleep 1
        done

        if [ -s $test_dir/peers_list_$1 ]; then
                cp $project_root/test_utils/ws_for_sync_finished.js $test_dir
                nodejs ws_for_sync_finished.js $1
                if [ "0" == "$?" ]; then
                        echo 0
                        return;
                else
                        echo "sync_finished doesn't receive" >> $test_dir/sync_finished_error
                fi
        fi
        echo 1
}

decrypt_user_json_info()
{
        tmp_file=$(mktemp)
        $project_root/utils/encryption_decryption/decrypt_user_json_info.sh $1/user/private.pem $1/user_json_info.json $tmp_file
        cat $tmp_file
        rm $tmp_file
}

get_user_json_uuids()
{
        for ((i=1; i<=$1; i++))
        do
                uj_uuids+=("$(decrypt_user_json_info $test_dir/keys${i} | jq '.uuid' | sed -e 's/^"//' -e 's/"$//')")
        done
        echo "user jsons ${uj_uuids[@]}"
}
