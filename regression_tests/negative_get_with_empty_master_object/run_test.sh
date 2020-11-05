#!/bin/bash

# Test case flow:
# - Runs one instance of metax.
# - Sends "node" save request for some content (default it is saved as encrypted).
# - Removes the content of master json.
# - Sends get request for received uuid.
# - Checks the compliance of received content and error message.


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0


main()
{
        $EXECUTE -f $tst_src_dir/config.xml &
        p=$!
        wait_for_init 7081
        echo -n  $p > $test_dir/processes_ids

        save_content="Some content for test"
        uuid=$(curl --data "$save_content" http://localhost:7081/db/save/node)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        rm $test_dir/storage/$uuid
        touch $test_dir/storage/$uuid
        get_content=$(curl -D $test_dir/h http://localhost:7081/db/get/?id=$uuid)
        echo $get_content
        res='{"error":"Getting file failed: uuid='$uuid'. Failed to decrypt data."}'
        if [ "$res" != "$get_content" ]; then
                echo "Mismatch of expected/received messages. get_content = $get_content, res = $res"
                kill_metax $p
                exit;
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
