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
	$EXECUTE -f $tst_src_dir/config.xml &
	p=$!
        wait_for_init 7081
        echo -n $p > $test_dir/processes_ids

        res=$(curl http://localhost:7081/config/get_user_public_key)
        echo "$res"
        kill_metax $p
        get_public_key=$(echo $res | jq -r '.user_public_key')
        echo "public_key=$get_public_key"

        if [ ! -s $test_dir/keys/user/public.pem ]; then
                echo "keys/user/public.pem is empty"
                exit;
        fi

        public_key=$(cat $test_dir/keys/user/public.pem)
        if [ "$get_public_key" != "$public_key" ]; then
                echo "Received incorrect public key. public_key = $public_key "
                exit;
        fi
        echo $PASS > $final_test_result
}

main $@
