#!/bin/bash
if [ ! "$1" ] ; then exit ; fi
hg up -C cleanup
yes d | hg merge -r "$1"
hg revert -a -r null
hg commit -m 'eliminating head'
hg status -u | xargs rm -rf
