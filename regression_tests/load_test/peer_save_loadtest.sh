#!/bin/bash

if [ -z "$1" ]
then
        echo "Please supply number of metax instances"
        exit
fi
N=$1

host=localhost

result_dir=result
source_dir=source
bin_path=$PWD/../../bin/metax_web_api
mport=9000
wport=9100
mpids=mpids
sout=sout

#list="3K 3M 7M 40M 120M 700M"
#list="3K 3M 7M 40M"
list="3K 3M 3M 3M 3M 3M 7M 40M 40M 40M 120M 120M 700M 3M 3M 3M 3M 3M"

function prepare_source_files
{
        mkdir -p $source_dir
        dd if=/dev/urandom of=$source_dir/3K count=3 bs=1024
        dd if=/dev/urandom of=$source_dir/3M count=3000 bs=1024
        dd if=/dev/urandom of=$source_dir/7M count=7000 bs=1024
        dd if=/dev/urandom of=$source_dir/40M count=40000 bs=1024
        dd if=/dev/urandom of=$source_dir/120M count=120000 bs=1024
        dd if=/dev/urandom of=$source_dir/700M count=700000 bs=1024
}

function prepare_metaxes
{
        n=$1
        echo preparing $n metax directories
        for ((i=1;  i <= $n; ++i))
        do
                mp=$((mport+i))
                wp=$((wport+i))
                metax_dir=$result_dir/metax$i
                mkdir -p $metax_dir
                sed -e "s/%MPORT%/$mp/" \
                        -e "s/%WPORT%/$wp/" \
                        -e "s/%PEER%/109.75.33.37:7082/" \
                        templates/config.xml > $metax_dir/config.xml
                #cp templates/peers_ratings.json $metax_dir/
                mkdir -p $metax_dir/bin
                ln -sf $bin_path $metax_dir/bin/test_metax
                # in case directories exist anyway remove log
                rm -f $metax_dir/leviathan.log*
        done
}

function launch_metaxes
{
        n=$1
        echo "launching $n instances of metax"
        for ((i=1;  i <= $n; ++i))
        do
                metax_dir=$result_dir/metax$i
                echo $metax_dir
                cd $metax_dir && exec ./bin/test_metax -f config.xml & echo -n " $!" >> $mpids
        done
}

prepare_source_files
mkdir -p $result_dir
prepare_metaxes $N
rm -f sout_*
rm -f $mpids
launch_metaxes $N
sleep 7

w=1
for ((i=1;  i <= $N; ++i))
do
        wp=$((wport+i))
        for f in $list
        do
                echo "(time curl --data \"$PWD/$source_dir/$f\" \"http://localhost:$wp/db/save/path?enc=0\" 2> /dev/null) >> ${sout}_$f 2<&1 &"
                (time curl --data "$PWD/$source_dir/$f" "http://localhost:$wp/db/save/path?enc=0" 2> /dev/null) >> ${sout}_$f 2<&1 &
                pids[$w]=$!
                echo $pids
                w=$((w+1))
        done
done

FAIL=0
for pid in ${pids[*]}
do
        wait $pid || FAIL=1
        if [ $FAIL != 0 ]; then
                echo "Failed save file in the host:" $host
                exit 1 
        fi
done

kill `cat $mpids`

grep "fail\|exception\|error" sout_*

if [[ 0 -eq $? ]]; then
        exit 1
fi
