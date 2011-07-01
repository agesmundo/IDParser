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
    chomp;
    my $ra_conll = conllrec_to_array($_);
    my $ra_conll07 = conllarray_to_conll07array($ra_conll);
    print conllarray_to_columnstring($ra_conll07);
}

print STDERR "$nrec CoNLL records read\n";


