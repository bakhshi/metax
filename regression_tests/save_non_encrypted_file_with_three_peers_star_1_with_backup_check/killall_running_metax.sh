p_id=$(cat $test_dir/processes_ids 2>/dev/null)
kill -9 $p_id &>/dev/null || true
