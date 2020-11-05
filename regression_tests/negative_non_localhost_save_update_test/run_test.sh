#!/bin/bash

# Test case flow:
# - Runs an instance of metax with config.xml configuration.
# - In config.xml "enable_non_localhost_save" is set "false".
# - Sends non-localhost "save" request. Checks the compliance of received/expected messages.
# - Sends localhost "save" request to get a valid uuid.Then sends non-localhost
#   "update" request. Checks the compliance of received/expected messages.

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
        $EXECUTE -f $tst_src_dir/config.xml &
        p=$!
        wait_for_init 7081
        echo -n $p > $test_dir/processes_ids

        res=$(curl -D $test_dir/h --data "save content" http://$ip:7081/db/save/node)
        expected='{"error": "Exception: With the current configuration it is not allowed to save/update data in non-localhost."}'
        echo "$res"
        if [ "$res" != "$expected" ]; then
                echo "Save response and expected message mismatch. Expected = $expected, Res = $res"
                kill_metax $p
                exit
        fi
        http_code=$(cat $test_dir/h | grep HTTP/1.1 | awk {'print $2'})
        echo "HTTP code of SAVE request response is $http_code"
        if [[ 400 != $http_code ]]; then
                echo "HTTP code of SAVE request response is $http_code"
                kill_metax $p
                exit;
        fi
        res=$(curl --data "save content" http://localhost:7081/db/save/node)
        echo $res
        uuid=$(echo $res | cut -d'"' -f 4)
        update=$(curl --data "updated content" http://$ip:7081/db/save/node?id=$uuid)
        echo $update
        if [ "$update" != "$expected" ]; then
                echo "Update response and expected message mismatch. Expected = $expected, update = $update"
                kill_metax $p
                exit
        fi
        http_code=$(cat $test_dir/h | grep HTTP/1.1 | awk {'print $2'})
        echo "HTTP code of UPDATE request response is $http_code"
        if [[ 400 != $http_code ]]; then
                echo "HTTP code of UPDATE request response is $http_code"
                kill_metax $p
                exit;
        fi

        echo $PASS > $final_test_result
	kill_metax $p
}

main $@

# vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:
