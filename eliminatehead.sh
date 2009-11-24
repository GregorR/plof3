#!/bin/bash -x
if [ ! "$1" ] ; then exit ; fi
hg up -C cleanup
CUR="`hg id -i`"
yes d | hg merge -r "$1"
hg revert -a -r "$CUR"
hg commit -m 'eliminating head'
hg status -u | xargs rm -rf
rm -rf */
