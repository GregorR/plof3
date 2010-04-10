#!/bin/bash

die() {
    echo "$1"
    exit 1
}

if [ ! "$1" ]
then
    echo 'Use: ./extcoreball.sh <coreball>'
    exit 1
fi
CB="$1"

tar jxf "$CB" hgid || die "Failed to extract hgid"
hg up -C `sed 's/[^a-f0-9].*//' hgid` || die "Failed to update to `cat hgid`"
tar jxf "$CB" || die "Failed to extract coreball"
gdb ./cplof/src/cplof core
