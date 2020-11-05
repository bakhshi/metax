#!/bin/bash

# Test case flow:
# - Rruns one instance of Metax
# - Sends get_metax_info request and checks that user_info_uuid is empty
# - Sends set_metax_info request and sets hard-coded uuid as metax_user_info uuid
# - Checks that response is OK, metax_user_info.json created and contains hard-coded uuid
# - Sends get_metax_info request
# - Check compliance of user_info_uuid, user_public_key and device_public_key
# - Kills Metax then launches again
# - Sends get_metax_info request
# - Checks that user_info_uuid is stored permanently

source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0


main()
{
	$EXECUTE -f $tst_src_dir/config.xml &
	p=$!
        is_http_server_started=$(wait_for_init 7081)
        if [ "0" != "$is_http_server_started" ];then
                echo "htpp server isn't started - 7081"
                kill_metax $p
                exit
        fi
        echo -n $p > $test_dir/processes_ids

        res=$(curl http://localhost:7081/config/get_metax_info)
        echo "set_metax_info request response before set is $res"
        get_user_info_uuid=$(echo $res | jq -r '.user_info_uuid')
        if [ -n "$get_user_info_uuid" ]; then
                echo "user_info_uuid before set should be empty"
                kill_metax $p
                exit
        fi

        uuid="4aadeec8-e30a-472a-acec-47266acec4a6"
        res=$(curl http://localhost:7081/config/set_metax_info?metax_user_uuid=$uuid)
        echo "set_metax_info request response is $res"
        expected='{"status":"OK"}'
        if [ "$expected" != "$res" ]; then
                echo "set_metax_info request failed"
                kill_metax $p
                exit
        fi

        if [ ! -s metax_user_info.json ]; then
                echo "Failed to create metax_user_info.json and/or set metax info uuid"
                kill_metax $p
                exit
        fi

        set_user_info_uuid=$(cat metax_user_info.json | jq -r '.user_info_uuid')
        if [ "$set_user_info_uuid" != "$uuid" ]; then
                echo "Failed to set user info uuid"
                kill_metax $p
                exit
        fi

        res=$(curl http://localhost:7081/config/get_metax_info)
        kill_metax $p

        echo "get_metax_info request response is $res"

        get_user_info_uuid=$(echo $res | jq -r '.user_info_uuid')
        if [ "$get_user_info_uuid" != "$uuid" ]; then
                echo "Failed to get user info uuid"
                exit
        fi

        get_user_public_key=$(echo $res | jq -r '.user_public_key')

        if [ ! -s $test_dir/keys/user/public.pem ]; then
                echo "keys/user/public.pem is empty"
                exit
        fi

        user_public_key=$(cat $test_dir/keys/user/public.pem)
        if [ "$get_user_public_key" != "$user_public_key" ]; then
                echo "Received incorrect user public key. user public_key = $user_public_key "
                exit
        fi

        get_device_public_key=$(echo $res | jq -r '.device_public_key')

        if [ ! -s $test_dir/keys/device/public.pem ]; then
                echo "keys/device/public.pem is empty"
                exit
        fi

        device_public_key=$(cat $test_dir/keys/device/public.pem)
        if [ "$get_device_public_key" != "$device_public_key" ]; then
                echo "Received incorrect device public key. device public_key = $device_public_key "
                exit
        fi

	$EXECUTE -f $tst_src_dir/config.xml &
	p=$!
        is_http_server_started=$(wait_for_init 7081)
        if [ "0" != "$is_http_server_started" ];then
                echo "htpp server isn't started - 7081"
                kill_metax $p
                exit
        fi
        echo -n $p > $test_dir/processes_ids

        res=$(curl http://localhost:7081/config/get_metax_info)
        kill_metax $p

        echo "get_metax_info request response after relaunch is $res"

        get_user_info_uuid=$(echo $res | jq -r '.user_info_uuid')
        if [ "$get_user_info_uuid" != "$uuid" ]; then
                echo "Failed to get user info uuid after relaunch"
                exit
        fi

        echo $PASS > $final_test_result
}

main $@
