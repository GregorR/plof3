#!/bin/bash
echo 0 > gctiming.dc
for (( run = 1; $run <= 10; run++ ))
do
    echo 'Run '$run
    cplof/src/cplof tests/test.plof > /dev/null
    (
        echo 0
        gprof cplof/src/cplof --flat-profile |
            grep GC_ |
            sed 's/ \+/ /g' |
            cut -d ' ' -f 2 |
            sed 's/$/ +/'
        echo p
    ) | dc | sed 's/$/ +/' >> gctiming.dc
done
echo '10 / p' >> gctiming.dc
cat gctiming.dc
dc < gctiming.dc
