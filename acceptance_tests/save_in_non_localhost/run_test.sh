#!/bin/bash

# Test case flow:
# - Runs two instances of metax
#        - in 1st, the enable_non_localhost_save config option is false
#        - in 2nd, the enable_non_localhost_save config option is true
# - Sends two save requests for each instance, where the host of the request is:
#       - localhost
#       - the ip address
# - When enable_non_localhost_save is false and the request is with ip address
#   then save request should fail, in other cases the save request should
#   succeed

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
        cd $test_dir
        $EXECUTE -f $tst_src_dir/config_non_localhost_save_false.xml &
        pid=$!
        is_http_server_started=$(wait_for_init 7081)
        if [ "0" != "$is_http_server_started" ];then
                echo "htpp server isn't started - 7081"
                exit
        fi
        echo -n $pid > processes_ids

        uuid=$(curl --data "test data" "http://localhost:7081/db/save/node?enc=0")
        uuid=$(echo $uuid | cut -d'"' -f 4)
        if [[ $uuid =~ [0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12} ]]; then
                echo "Save succeeded"
        else
                echo "Error during save"
                kill_metax $pid
                exit
        fi
        uuid=$(curl --data "test data" "http://$ip:7081/db/save/node?enc=0")
        uuid=$(echo $uuid | cut -d'"' -f 4)
        if [[ $uuid =~ [0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12} ]]; then
                echo "Save succeeded, but shouldn't"
                kill_metax $pid
                exit
        else
                echo $uuid
        fi

        kill_metax $pid
        $EXECUTE -f $tst_src_dir/config_non_localhost_save_true.xml &
        pid=$!
        is_http_server_started=$(wait_for_init 7081)
        if [ "0" != "$is_http_server_started" ];then
                echo "htpp server isn't started - 7081"
                exit
        fi
        echo -n " $pid" >> processes_ids

        uuid=$(curl --data "test data" "http://localhost:7081/db/save/node?enc=0")
        uuid=$(echo $uuid | cut -d'"' -f 4)
        if [[ $uuid =~ [0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12} ]]; then
                echo "Save succeeded"
        else
                echo "Error during save"
                kill_metax $pid
                exit
        fi
        uuid=$(curl --data "test data" "http://$ip:7081/db/save/node?enc=0")
        uuid=$(echo $uuid | cut -d'"' -f 4)
        if [[ $uuid =~ [0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12} ]]; then
                echo "Save succeeded"
        else
                echo "Error during save"
                kill_metax $pid
                exit
        fi
        echo $PASS > $final_test_result
        kill_metax $pid
}

main $@
