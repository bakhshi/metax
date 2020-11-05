p_id=$(cat $test_dir/processes_ids)
kill -9 $p_id &>/dev/null || true
