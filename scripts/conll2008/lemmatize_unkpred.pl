#!/usr/bin/perl
use strict;
use warnings;

my $nrec = 0;
my $nunk = 0;
while (<>){
    if (/^\s*$/){
	$nrec++; next;
    }
    chomp;
    my @afield = split /\s+/;
    next if ($afield[10] eq '_'); # PRED => 10
    my $pred = $afield[10];
    $pred =~ s/^([^\.]+)\.\d+$/$1/;
    if ($pred ne $afield[6]){
	print STDERR "$nrec $afield[2] $pred\n";
	$nunk++;
    }
}
print STDERR "$nunk unknown predicates\n";
