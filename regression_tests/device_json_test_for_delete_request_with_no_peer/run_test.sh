#!/bin/bash

# Test case flow: 
# - Run one instances of metax.
# - Sends "node" save request for some content (default it is saved as encrypted)
# - Checks the device.json validation.
# - Sends "delete" request for saved data.
# - Checks the device.json validation after delete requests.

source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0
save_content="Save some content for test"
re='^[0-9]+$'




main()
{
	$EXECUTE -f $tst_src_dir/config.xml &
        p=$!
        wait_for_init 7081
        echo -n $p > $test_dir/processes_ids

        uj_uuid=$(decrypt_user_json_info $test_dir/keys | jq '.uuid' | sed -e 's/^"//' -e 's/"$//')

        uuid=$(curl --data "$save_content" "http://localhost:7081/db/save/node")
        uuid=$(echo $uuid | cut -d'"' -f 4)
        get_content=$(curl "http://localhost:7081/db/get/?id=$uuid")
        kill_metax $p
        if [ "$save_content" != "$get_content" ]; then
                echo "Getting saved data failed"
                exit;
        fi

        uuids=$(ls $test_dir/storage/????????-* | grep -v "$uj_uuid\|$uj_uuid.tmp" | rev | cut -d'/' -f 1 | rev)
        count=$(echo "$#uuids[*]}")
        #check the count of uuids in storage
        if [ $count == 0 ]; then
                echo count=$count
                echo "Storage is empty"
                exit;
        fi

        ../../utils/encryption_decryption/decrypt_user_json.sh $test_dir/keys/user/private.pem $test_dir/keys/user_json_info.json $test_dir/device.json $test_dir/decrypted_device.json
        storage=$(cat $test_dir/decrypted_device.json | jq -r '.storage')

        for i in $uuids
        do
                piece=$(echo $storage | jq -r ".\"$i\"");
                if [[ $piece == null ]]; then
                        echo "$i uuid missing in device.json"
                        exit;
                fi
                expiration_date=$(echo $piece | jq -r '.expiration_date');
                is_deleted=$(echo $piece | jq -r '.is_deleted');
                last_updated=$(echo $piece | jq -r '.last_updated');
                version=$(echo $piece | jq -r '.version');

                if [[ ! $expiration_date =~ $re || ! $last_updated =~ $re || ! $version =~ $re ]]; then
                        echo "Invalid properties in device.json (expiration_date=$expiration_date, last_updated=$last_updated, version=$version, uuid=$i)"
                        exit;
                fi
                if [ $is_deleted != "false" ]; then
                        echo "Invalid is_deleted=$is_deleted property for $i uuid in device.json"
                        exit;
                fi
        done

        $EXECUTE -f $tst_src_dir/config.xml &
	p=$!
        wait_for_init 7081
        echo -n $p > $test_dir/processes_ids

        expected='{"deleted":"'$uuid'"}'
        res=$(curl http://localhost:7081/db/delete?id=$uuid)
        kill_metax $p

        if [ "$expected" != "$res" ]; then
                echo "Failed to delete. Response is $res"
                exit;
        fi

        ../../utils/encryption_decryption/decrypt_user_json.sh $test_dir/keys/user/private.pem $test_dir/keys/user_json_info.json $test_dir/device.json $test_dir/decrypted_device.json
        storage=$(cat $test_dir/decrypted_device.json | jq -r '.storage')

        for i in $uuids
        do
                is_deleted1=$(echo $storage | jq -r ".\"$i\"" | jq -r '.is_deleted')
                last_updated1=$(echo $storage | jq -r ".\"$i\"" | jq -r '.last_updated');
                version1=$(echo $storage | jq -r ".\"$i\"" | jq -r '.version');
                if [[ ! $last_updated1 =~ $re ]] || [ "$last_updated1" -lt "$last_updated" ]; then
                        echo "Invalid properties in device.json (last_updated=$last_updated, uuid=$i)"
                        exit;
                fi
                if [ "$version1" -lt "$version" ]; then
                        echo "Invalid version=$version for $i uuid"
                        exit;
                fi
                if [ $is_deleted1 != "true" ]; then
                        echo "Invalid is_deleted=$is_deleted for $i uuid"
                        exit;
                fi
        done
        echo $PASS > $final_test_result
}

main $@


