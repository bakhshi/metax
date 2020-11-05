#!/bin/bash

# Test case flow:
# - Run one instances of metax.
# - Sends "node" save request for some content (default it is saved as encrypted)
# - Checks validation of device.json
# - Sends "update" request for saved data
# - Checks validation of device.json after update request.

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
                echo "Getting saved data by peer failed"
                exit;
        fi

        uuids1=$(ls  $test_dir/storage/????????-* | grep -v "$uj_uuid\|$uj_uuid.tmp" | rev | cut -d'/' -f 1 | rev)
        count=$(echo "${#uuids1[*]}")
        #check the count of uuids in storage
        if [ $count == 0 ]; then
                echo "Storage is empty"
                exit;
        fi

        ../../utils/encryption_decryption/decrypt_user_json.sh $test_dir/keys/user/private.pem $test_dir/keys/user_json_info.json $test_dir/device.json $test_dir/decrypted_device.json
        storage=$(cat $test_dir/decrypted_device.json | jq -r '.storage')

        for i in $uuids1
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

                if [[ ! $expiration_date =~ $re ]] || [[ ! $last_updated =~ $re ]]; then
                        echo "Invalid properties in device.json"
                        exit;
                fi
                if [ "$is_deleted" != "false" ]; then
                        echo "Invalid 'is_deleted' key in device.json"
                        exit;
                fi
                if (("$version" < 1)); then
                        echo "Invalid version in device.json"
                        exit;
                fi
        done

        # keep properties for master object in device json before update request.
        expiration_date=$(echo $storage | jq -r ".\"$uuid\"" | jq -r '.expiration_date')
        is_deleted=$(echo $storage | jq -r ".\"$uuid\"" | jq -r '.is_deleted')
        last_updated=$(echo $storage | jq -r ".\"$uuid\"" | jq -r '.last_updated')
        version=$(echo $storage | jq -r ".\"$uuid\"" | jq -r '.version')

        update_content="Test message for content update test"
        $EXECUTE -f $tst_src_dir/config.xml &
        p=$!
        wait_for_init 7081
        echo -n $p > $test_dir/processes_ids

        uuid1=$(curl --data "$update_content" http://localhost:7081/db/save/node?id=$uuid)
        uuid1=$(echo $uuid1 | cut -d'"' -f 4)
        kill_metax $p

        ../../utils/encryption_decryption/decrypt_user_json.sh $test_dir/keys/user/private.pem $test_dir/keys/user_json_info.json $test_dir/device.json $test_dir/decrypted_device.json
        storage=$(cat $test_dir/decrypted_device.json | jq -r '.storage')
        uuids2=$(echo $storage | jq -r 'keys[]' )

        #Keep propertieces for updated uuid.
        is_deleted1=$(echo $storage | jq -r ".\"$uuid\"" | jq -r '.is_deleted')
        last_updated1=$(echo $storage | jq -r ".\"$uuid\"" | jq -r '.last_updated')
        version1=$(echo $storage | jq -r ".\"$uuid\"" | jq -r '.version')
        ((version++))

        if [[ ! $last_updated1 =~ $re ]] || [ "$last_updated1" -lt "$last_updated" ]; then
                echo "Invalid last_updated1=$last_updated1 key in device.json after update"
                exit;
        fi
        if [[ "$is_deleted1" -ne "false" ]];then
                echo "Invalid is_deleted1=$is_deleted1 key in device.json after update"
                exit;
        fi
        if [ "$version1" != "$version" ];then
                echo "Invalid version1=$version1 key in device.json after update"
                exit;
        fi

        for i in $uuids2
        do
                if [ "$i" == "$uuid" ]; then
                        # The case of master object is already checked.
                        continue;
                fi
                is_deleted2=$(echo $storage | jq -r ".\"$i\"" | jq -r '.is_deleted')
                last_updated2=$(echo $storage | jq -r ".\"$i\"" | jq -r '.last_updated')
                version2=$(echo $storage | jq -r ".\"$i\"" | jq -r '.version')

                if [[ "${uuids1[@]}" =~ "$i" ]]; then
                        if [[ ! $last_updated2 =~ $re ]] || [ "$last_updated2" -lt "$last_updated" ]; then
                                echo "Invalid last_updated=$last_updated2 field for old pieces"
                                exit;
                        fi
                        if [ "$is_deleted2" != "true" ]; then
                                echo "Invalid is_deleted=$is_deleted2 field for old pieces"
                                exit;
                        fi
                        if (($version2 < 2)); then
                                echo "Invalid version=$version2 field for old pieces"
                                exit;
                        fi
                else
                        if [[ ! $last_updated2 =~ $re ]] || [ "$last_updated2" -lt "$last_updated" ]; then
                                echo "Invalid last_updated=$last_updated2 field for new pieces"
                                exit;
                        fi
                        if [ "$is_deleted2" != "false" ]; then
                                echo "Invalid is_deleted=$is_deleted2 field for new pieces"
                                exit;
                        fi
                        if (($version2 < 1)); then
                                echo "Invalid version=$version2 field for new pieces"
                                exit;
                        fi
                fi
        done
        echo $PASS > $final_test_result

}
main $@


