#!/bin/bash

# Test case flow:
# - Runs one instance of metax.
# - Sends "node" save request for some content with enc=0 parameter.
# - Sends get request for received uuid.
# - Checks the compliance of received/saved content.

save_content="Some content for test"


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0


main()
{
        $EXECUTE -f $tst_src_dir/config.xml &
        p=$!
        wait_for_init 7081
        echo -n $p > $test_dir/processes_ids

        expected='{"error":"Failed to save."}'
        res=$(curl -D $test_dir/h --data "$save_content" http://localhost:7081/db/save/node?local=0)
        echo $res
        echo "Response is $res"
        if [ "$expected" != "$res" ]; then
                echo "Mismatch of expected/received messages. Res = $res, Rxpected = $expected"
                kill_metax $p
                exit;
        fi
        http_code=$(cat $test_dir/h | grep HTTP/1.1 | awk {'print $2'})
        echo "HTTP code of SAVE request response is $http_code"
        if [[ 404 != $http_code ]]; then
                echo "HTTP code of SAVE request response is $http_code"
                kill_metax $p
                exit;
        fi
        echo $PASS > $final_test_result;
        kill_metax $p
}

main $@
