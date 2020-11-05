#!/bin/bash

# Test case flow:
# - Runs an instance of metax with config.xml configuration without any peer.
# - Sends "save" request for getting a valid uuid.
# - Greps "master_uuid" in the shared object.
# - Sends "get" request with uuid, which is not a shared object.
# - Checks the compliance of received/expected messages.


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0


main()
{
        $EXECUTE -f $tst_src_dir/config.xml &
        p=$!
        wait_for_init 7081
        echo -n $p > $test_dir/processes_ids

        save=$(curl --data "save content" http://localhost:7081/db/save/node)
        uuid=$(echo $save | cut -d'"' -f 4)
        echo 'master_uuid='$uuid''
        rm $test_dir/storage/$uuid
        echo '{"invalid":"master"}' > $test_dir/storage/$uuid
        res=$(curl -D $test_dir/h http://localhost:7081/db/get?id=$uuid)
        echo $res
        expected='{"error":"Getting file failed: uuid='$uuid'. Failed to decrypt data."}'
        if [ "$res" != "$expected" ]; then
                echo "Mismatch of expected/received messages. Expected = $expected, Res = $res"
                kill_metax $p
                exit
        fi
        http_code=$(cat $test_dir/h | grep HTTP/1.1 | awk {'print $2'})
        echo "HTTP code of GET request response is $http_code"
        if [[ 404 != $http_code ]]; then
                echo "HTTP code of GET request response is $http_code"
                kill_metax $p
                exit;
        fi

        echo $PASS > $final_test_result
	kill_metax $p
}

main $@

# vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:
