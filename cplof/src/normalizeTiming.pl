#!/usr/bin/perl

$last = "";
$count = 0;
$total = 0;

while ($line = <>) {
    @elems = split(/,/, $line);
    if (!($elems[0] eq $last)) {
        print "$last,$count,$total\n";
        $last = $elems[0];
        $count = 0;
        $total = 0;
    }
    $count++;
    $total += $elems[1];
}
print "$last,$count,$total\n";
