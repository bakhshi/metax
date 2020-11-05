#!/bin/bash

# Test case flow:
# - Runs an instance of metax with config.xml configuration.
# - Sends "save" request for getting a valid uuid.
# - Removes one uuid, which is not a shared object or master uuid, from the
#   storage.
# - Sends "get" request to get an object, which chunk is missing.
# - Checks the compliance of received/expected messages.

rm -rf $test_dir/storage
save_file=$test_dir/save_file


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0


main()
{
        # Generate ~8MB random file
        dd if=/dev/urandom of=$save_file bs=8000000 count=1

        $EXECUTE -f $tst_src_dir/config.xml &
        p=$!
        wait_for_init 7081
        echo -n $p > $test_dir/processes_ids

        uj_uuid=$(decrypt_user_json_info $test_dir/keys | jq '.uuid' | sed -e 's/^"//' -e 's/"$//')
        echo "uj_uuid is $uj_uuid"

        save=$(curl --data "$save_file" http://localhost:7081/db/save/path)
        uuid=$(echo $save | cut -d'"' -f 4)
        echo $uuid
        echo $(ls $test_dir/storage/????????-*)

        to_del=$(ls $test_dir/storage/????????-* | grep -v $uuid | grep -v "$uj_uuid\|$uj_uuid.tmp" | head -n 1)
        echo $to_del
        rm -v $to_del
        if [ "0" != "$?" ];then
                echo "Failed to remove uuid."
                kill_metax $p
                exit;
        fi

        res=$(curl -D $test_dir/h -m 10 http://localhost:7081/db/get?id=$uuid)
        chunk_uuid="$(basename $to_del)"
        echo $chunk_uuid
        echo $res
        expected='{"error":"Getting file failed: uuid='$uuid'. There is a missing chunk in the requested object: chunk_uuid = '$chunk_uuid'"}'
        if [ "$res" != "$expected" ]; then
                echo "Mismatch of expected/received messages. Res = $res, Rxpected = $expected"
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
