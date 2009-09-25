#!/bin/bash
echo '5 k 0' > runtiming.dc
for (( run = 1; $run <= 10; run++ ))
do
    echo 'Run '$run
    time -p cplof/src/cplof tests/test.plof 2>&1 |
        grep '^real' |
        sed 's/^.* // ; s/$/ +/' >> runtiming.dc
done
echo '10 / p' >> runtiming.dc
cat runtiming.dc
dc < runtiming.dc
