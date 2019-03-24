#!/bin/sh

dest_dir=ar_merge_$$_`date +%s`
mkdir -p $dest_dir/libs $dest_dir/objs $dest_dir/tmp

opwd=`pwd`
_e()
{
    cd $opwd
    rm -rf $dest_dir
}

dest_a=$1

while [ "$1" != "" ]
do
    cp -a $1 $dest_dir/libs/
    shift
done

cd $dest_dir/tmp || {
    echo error: cd $dest_dir/tmp
    _e
    exit 1
}

did=`date +%s`
sn=1
for lib in ../libs/*
do
    sn=` expr $sn + 1 `
    ar x $lib
    for obj in *
    do
        mv $obj  ../objs/"$sn"_"$did"_$obj
    done
done

cd $opwd
ar r $dest_a $dest_dir/objs/*
ranlib $dest_a

_e
