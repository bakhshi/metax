#!/bin/bash

# Test case flow:
# - runs one instance of Metax
# - saves encrypted data 25 times
# - decrypts user json then checked pieces.


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0


main()
{
	$EXECUTE -f $tst_src_dir/config.xml &
	p=$!
        wait_for_init 7081
        echo -n $p > $test_dir/processes_ids

        declare -a uuids
        for i in `seq 1 25`;
        do
                res=$(curl --data "some save data" http://localhost:7081/db/save/node)
                uuid=$(echo $res | cut -d'"' -f 4)
                uuids[$i]=$uuid
        done
        kill_metax $p

        uj_uuid=$(decrypt_user_json_info $test_dir/keys | jq '.uuid' | sed -e 's/^"//' -e 's/"$//')
        echo user_json uuid is $uj_uuid

        ../../utils/encryption_decryption/decrypt_user_json.sh $test_dir/keys//user/private.pem $test_dir/keys/user_json_info.json $test_dir/storage/$uj_uuid $test_dir/decrypted_user.json
        if [ ! -s $test_dir/decrypted_user.json ]; then
                echo "decrypted_user.json is empty"
                exit;
        fi
        owned_data=$(cat $test_dir/decrypted_user.json | jq -r '.owned_data')
        for i in "${uuids[@]}"
        do
                piece=$(echo $owned_data | jq -r ".\"$i\"");
                if [[ $piece == null ]]; then
                        echo "$i uuid missing in user.json"
                        exit;
                fi
        done
        echo $PASS > $final_test_result
}

main $@
