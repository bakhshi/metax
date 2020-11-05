#!/bin/bash

# Test case flow: 
# - runs two connected Metax peers 1 and 2
# - saves encrypted data in peer 1
# - shares whith username of peer 2
# - gets appropriate error message while public key of peer 2 is not available
#   for peer 1
# - add peer 2 public key using add_peer API
# - shares the saved uuid again
# - share successes
# - peer 2 gets and checks the save data


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0


main()
{
	security_token=12345
        $EXECUTE -f $tst_src_dir/config.xml -u $security_token &
	p=$!
        is_http_server_started=$(wait_for_init 7081)
        if [ "0" != "$is_http_server_started" ];then
                echo "htpp server isn't started - 7081"
                kill_metax $p
                exit
        fi
        echo -n $p > $test_dir/processes_ids
        res=$(curl http://localhost:7081/config/get_online_peers?token=$security_token)
        echo $res
	expected="[]"
        if [ "$res" != "$expected" ]; then
                echo "security token check failed"
                kill_metax $p
                exit
        fi
        echo $PASS > $final_test_result
	kill_metax $p
}

main $@
