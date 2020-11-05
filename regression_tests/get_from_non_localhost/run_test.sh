#!/bin/bash

# Test case flow:
# - Runs Metax with the enable_non_localhost_get config option set to true
# - Saves some data
# - Sends two get requests from localhost and by the IP address
# - Checks that files were got
# - Kills Metax
# - Runs Metax with the enable_non_localhost_get config option set to false
# - Sends two get requests from localhost and by the IP address
# - Checks that file have been got from localhost and not by the IP address


# works also for Mac OS
if hash ifconfig 2>/dev/null; then
        ip=$(ifconfig | grep "inet\>" | grep -v 127.0.0.1 | awk '{print $2}' | cut -d':' -f2 | head -n1)
else
        ip=$(ip addr | grep "inet\>" | grep -v 127.0.0.1 | awk '{print $2}' | cut -d'/' -f1 | head -n1)
fi


source ../../test_utils/functions.sh


# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0


main()
{
        $EXECUTE -f $tst_src_dir/config_non_localhost_get_true.xml &
        pid=$!
        is_http_server_started=$(wait_for_init 7081)
        if [ "0" != "$is_http_server_started" ];then
                echo "htpp server isn't started - 7081"
                exit
        fi
        echo -n $pid > processes_ids

        some_string="test_data"
        save_res=$(curl --data $some_string "http://localhost:7081/db/save/node?enc=0")
        uuid=$(echo $save_res | cut -d'"' -f 2)
        if [[ "uuid" != "$uuid" ]]; then
                echo "Failed to save"
                echo "Response is $save_res"
                echo $FAIL > $final_test_result
                kill_metax $pid
                exit
        fi

        uuid=$(echo $save_res | cut -d'"' -f 4)
        echo "uuid is $uuid"

        get_from_localhost=$(curl http://localhost:7081/db/get?id=$uuid)
        get_by_ip=$(curl http://$ip:7081/db/get?id=$uuid)

        if [[ $some_string != $get_from_localhost ]]; then
                echo "Failed to get saved data from localhost when enable_non_localhost_get is true"
                echo "Response is $get_from_localhost"
                echo $FAIL > $final_test_result
                kill_metax $pid
                exit
        fi
        if [[ $some_string != $get_by_ip ]]; then
                echo "Failed to get saved data from non-localhost when enable_non_localhost_get is true"
                echo "Response is $get_by_ip"
                echo $FAIL > $final_test_result
                kill_metax $pid
                exit
        fi

        kill_metax $pid
        $EXECUTE -f $tst_src_dir/config_non_localhost_get_false.xml &
        pid=$!
        is_http_server_started=$(wait_for_init 7081)
        if [ "0" != "$is_http_server_started" ];then
                echo "htpp server isn't started - 7081"
                exit
        fi
        echo -n " $pid" >> processes_ids

        get_from_localhost=$(curl http://localhost:7081/db/get?id=$uuid)
        get_by_ip=$(curl http://$ip:7081/db/get?id=$uuid)

        if [[ $some_string != $get_from_localhost ]]; then
                echo "Failed to get saved data from localhost when enable_non_localhost_get is false"
                echo "Response is $get_from_localhost"
                echo $FAIL > $final_test_result
                kill_metax $pid
                exit
        fi
        expected='{"error": "Exception: With the current configuration it is not allowed to get data from non-localhost."}'
        if [[ $expected != $get_by_ip ]]; then
                echo "Did not receive expected error when enable_non_localhost_get is false"
                echo "Expected response is $expected, but response is $get_by_ip"
                echo $FAIL > $final_test_result
                kill_metax $pid
                exit
        fi
        echo $PASS > $final_test_result
        kill_metax $pid
}

main $@
