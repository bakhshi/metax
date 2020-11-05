#!/bin/bash

# Test case flow: 
# - Run instances of metax.
# - Sends "node" save request for some content (default it is saved as encrypted)
# - Sends "copy" request for saved data. 
# - Checks device json validation.

source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0
save_content="Some content for test"
re='^[0-9]+$'



main()
{
	$EXECUTE -f $tst_src_dir/config.xml &
	p=$!
        wait_for_init 7081
        echo -n $p > $test_dir/processes_ids

        uj_uuid=$(decrypt_user_json_info $test_dir/keys | jq '.uuid' | sed -e 's/^"//' -e 's/"$//')
        echo "user json $uj_uuid"

        uuid=$(curl --data "$save_content" "http://localhost:7081/db/save/node")
        uuid=$(echo $uuid | cut -d'"' -f 4)
        get_content=$(curl "http://localhost:7081/db/get/?id=$uuid")
        if [ "$save_content" != "$get_content" ]; then
                echo "Getting saved data failed"
                kill_metax $p
                exit;
        fi
        uuid1=$(curl http://localhost:7081/db/copy?id=$uuid)
        uuid1=$(echo $uuid1 | jq -r '.copy_uuid')
        get_content=$(curl "http://localhost:7081/db/get/?id=$uuid1")

        if [ "$save_content" != "$get_content" ]; then
                echo "Getting copied data failed"
                kill_metax $p
                exit;
        fi

        kill_metax $p

        #check the count of uuids in storage
        uuids=$(ls  $test_dir/storage/????????-* | grep -v "$uj_uuid" | rev | cut -d'/' -f 1 | rev)

        ../../utils/encryption_decryption/decrypt_user_json.sh $test_dir/keys//user/private.pem $test_dir/keys/user_json_info.json $test_dir/device.json $test_dir/decrypted_device.json
        storage=$(cat $test_dir/decrypted_device.json | jq -r '.storage')

        for i in $uuids
        do
                piece=$(echo $storage | jq -r ".\"$i\"");
                if [[ $piece == null ]]; then
                        echo "$i uuid missing in user.json"
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
                if [ "$is_deleted" != "false" ]; then
                        echo "Invalid is_deleted=$is_deleted filed for pieces"
                        exit;
                fi
        done

        echo $PASS > $final_test_result
}

main $@
