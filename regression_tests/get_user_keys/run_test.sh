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
	$EXECUTE -f $tst_src_dir/config1.xml &
	p1=$!
        wait_for_init 7081
        echo -n $p1 > $test_dir/processes_ids

        res=$(curl http://localhost:7081/config/get_user_keys)
        kill_metax $p1 $p2;
        echo $res
        public_key=$(echo $res | jq -r '.user_public_key')
        private_key=$(echo $res | jq -r '.user_private_key')
        user_json_info=$(echo $res | jq -r '.user_json_key' | jq -c .)
        echo "user_json_info=$user_json_info"
        expected_public_key=$(cat $test_dir/keys1/user/public.pem)
        expected_private_key=$(cat $test_dir/keys1/user/private.pem)
        expected_user_info=$(decrypt_user_json_info $test_dir/keys1 | jq -c .)
        echo "expected_user_json_info=$expected_user_info"
        if [[ "$public_key" != "$expected_public_key" ]] || [[ "$private_key" != "$expected_private_key" ]] || [[ "$user_json_info" != "$expected_user_info" ]]; then
                echo "unexpected keys is received"
                exit;
        fi
        echo $PASS > $final_test_result
}

main $@
