#!/usr/bin/perl
use warnings;
use strict;
use CoNLL;
use Carp;

my $conllfile = shift;
open CONLL, "< $conllfile" or die "couldnt open file $conllfile: $!\n";
my $conll07file = shift;
open CONLL07, "< $conll07file" or die "couldnt open file $conllfile: $!\n";


$/ = "";
my $nrec = 0;
while (defined(my $_conll = <CONLL>) && defined(my $_conll07 = <CONLL07>)){
    $nrec++;
    chomp $_conll; chomp $_conll07;
    my $ra_conll = conllrec_to_array($_conll);
    my $ra_conll07 = conllrec_to_array($_conll07);
    my $ra_conllSUrm = conll07array_to_conllarraySUrm($ra_conll07, $ra_conll);
    # print conllarray_to_rowstring($ra_conll);
    print conllarray_to_columnstring($ra_conllSUrm);
    # print conllarray_to_columnstring($ra_conll);
    # last;
}

print STDERR "$nrec CoNLL records read\n";
