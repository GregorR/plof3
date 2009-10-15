#!/usr/bin/perl -w
use strict;

my %counts = ();
my %times = ();

while (my $line = <stdin>) {
    chomp $line;
    my @elems = split(/,/, $line);
    my $nm = $elems[0];
    my $tm = $elems[1] + 0;
    if (!($counts{$nm})) {
        $counts{$nm} = $times{$nm} = 0;
    }
    $counts{$nm}++;
    $times{$nm} += $tm;
}

my $nm;
foreach $nm (keys %counts) {
    my $count = $counts{$nm};
    my $time = $times{$nm};
    print "$nm,$count,$time\n";
}
