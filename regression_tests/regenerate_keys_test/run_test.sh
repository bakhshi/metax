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

        expected='{"status":"succeeded"}'
        old_public=$(cat $test_dir/keys/user/public.pem)
        old_private=$(cat $test_dir/keys/user/private.pem)
        old_user_json_info=$(decrypt_user_json_info $test_dir/keys)
        res=$(curl http://localhost:7081/config/regenerate_user_keys)
        echo "Response is $res"
        if [ "$expected" != "$res" ]; then
                echo "Invalid response"
                kill_metax $p
                exit;
        fi

        new_public=$(cat $test_dir/keys/user/public.pem)
        new_private=$(cat $test_dir/keys/user/private.pem)
        new_user_json_info=$(decrypt_user_json_info $test_dir/keys)

        # Check that keys are updated.
        if [[ "$old_public" == "$new_public" ]] || [[ "$old_private" == "$new_private" ]] || [[ "$old_user_json_info" == "$new_user_json_info" ]]; then
                echo "Keys are not updated successfully"
                kill_metax $p
                exit;
        fi

        # Check that device and user jsons are encrypted with new keys or not
        ../../utils/encryption_decryption/decrypt_user_json.sh $test_dir/keys/user/private.pem $test_dir/keys/user_json_info.json $test_dir/device.json $test_dir/decrypted_device.json
        p1=$?
        if [[ "0" != "$p1" ]]; then
                echo "Failed to decrypt device json with new key. Exit code - $p"
                kill_metax $p
                exit;
        fi
        ../../utils/encryption_decryption/decrypt_user_json.sh $test_dir/keys/user/private.pem $test_dir/keys/user_json_info.json $test_dir/user.json $test_dir/decrypted_user.json
        p1=$?
        if [[ "0" != "$p1" ]]; then
                echo "Failed to decrypt user json with new key. Exit code - $p"
                kill_metax $p
                exit;
        fi

        echo $PASS > $final_test_result;
        kill_metax $p
}

main $@
