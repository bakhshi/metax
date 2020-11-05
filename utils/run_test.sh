#!/bin/bash

result_file=result.txt

test_suite=regression_tests
#test_suite=acceptance_tests
#test_suite=continous_tests

if [ -z "$1" ]
then
      echo "no test specified"
      echo "usage: ./run_test.sh <test_name>"
      echo "       ./run_test.sh $test_suite/<test_name>"
      exit 1
fi

test=$(basename $1)

res=PASS

#while [ "$res" != "CRASH" ]
#do
        rm -fr ${test_suite}_results/$test
        #make test use_valgrind=yes test_type=$test_suite sub_tests_results=${test_suite}_results/$test/$result_file
        make test test_type=$test_suite sub_tests_results=${test_suite}_results/$test/$result_file
        res=$(cat ${test_suite}_results/$test/$result_file)
        echo $res
#done

