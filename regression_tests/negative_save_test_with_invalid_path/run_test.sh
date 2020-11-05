#!/bin/bash

# Test case flow:
# - Runs an instances of metax with config.xml configuration.
# - Sends "path" save request with invalid file path for some random
#   generated file (default it will be saved as encrypted).
# - Checks the compliance of received/expected messages.

save_file=$test_dir/save_file


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0



main()
{
        $EXECUTE -f $tst_src_dir/config.xml &
        p=$!
        wait_for_init 7081
        echo -n $p > $test_dir/processes_ids

        #dd-"data dublicator". Dublicates random data in save_file
        #according to the given options.
        dd if=/dev/urandom of=$save_file bs=1000000 count=1
        # specifying not valid file path
        res=$(curl -D $test_dir/h --data "/home/$save_file" http://localhost:7081/db/save/path)
        echo $res
        expected='{"error": "Exception: Invalid file path."}'
        if [ "$res" != "$expected" ]; then
                echo "Mismatch of expected/received messages. Res = $res, Rxpected = $expected"
                kill_metax $p
                exit;
        fi
        http_code=$(cat $test_dir/h | grep HTTP/1.1 | awk {'print $2'})
        echo "HTTP code of SAVE request response is $http_code"
        if [[ 400 != $http_code ]]; then
                echo "HTTP code of SAVE request response is $http_code"
                kill_metax $p
                exit;
        fi
        echo $PASS > $final_test_result
        kill_metax $p
}

main $@

# vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:
