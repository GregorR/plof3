#!/bin/bash
if [ ! "$1" ] ; then exit ; fi
hg up -C "$1"
hg commit --close-branch -m 'closing branch'
hg up -C cleanup
