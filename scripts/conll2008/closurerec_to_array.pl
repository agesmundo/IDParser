#!/usr/bin/perl
use warnings;
use strict;
use CoNLL;
use Carp;

my $conllfile = shift;
open CONLL, "< $conllfile" or die "couldnt open file $conllfile: $!\n";

$/ = "";
my $nrec = 0;

while (<CONLL>){
    $nrec++;
    chomp $_;
    my $ra_closure = closurerec_to_array($_);
    print closure_to_string($ra_closure);
}

print STDERR "$nrec CoNLL records read\n";
