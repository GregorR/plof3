#!/bin/sh
autoreconf
find . -name 'autom4te.cache' | xargs rm -rf
