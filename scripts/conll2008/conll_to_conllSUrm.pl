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
    next if /^\s*$/;
    $nrec++;
    chomp;
    my $ra_conll = conllrec_to_array($_);
    my $ra_conllSUrm = conllarray_to_conllarraySUrm($ra_conll); # rm SU predicates and SU arguments
    # print conllarray_to_rowstring($ra_conll);
    print conllarray_to_columnstring($ra_conllSUrm);
    # print conllarray_to_columnstring($ra_conll);
    # last;
}

print STDERR "$nrec CoNLL records read\n";
