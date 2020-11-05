#!/bin/bash

# Test case flow:
# - Runs an instance of metax with config.xml configuration without any peer.
# - Sends "save" request for getting a valid uuid. Then sends "update" request
#   with valid existing uuid. Checks the compliance of "save"/"update" request's messages.
# - Sends "update" request with non-existing uuid.
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
        updated_content="updated content"
        update=$(curl --data "$updated_content" http://localhost:7081/db/save/node?id=$uuid)
        echo $update
        if [ "$save" != "$update" ]; then
                echo "Failed updating with valid uuid. Save=$save, update=$update"
                kill_metax $p
                exit
        fi
        expected='{"error":"Failed updating uuid=d6f23daa-3733-46f8-8d84-713481150ea6. UUID is not found."}'
        res=$(curl -D $test_dir/h --data "$updated_content" http://localhost:7081/db/save/node?id=d6f23daa-3733-46f8-8d84-713481150ea6)
        echo $res
        if [ "$res" != "$expected" ]; then
                echo "Mismatch of expected/received messages. expected = $expected, Res = $res"
                kill_metax $p
                exit
        fi
        http_code=$(cat $test_dir/h | grep HTTP/1.1 | awk {'print $2'})
        echo "HTTP code of UPDATE request response is $http_code"
        if [[ 404 != $http_code ]]; then
                echo "HTTP code of UPDATE request response is $http_code"
                kill_metax $p
                exit;
        fi

        echo $PASS > $final_test_result
	kill_metax $p
}

main $@

# vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:
