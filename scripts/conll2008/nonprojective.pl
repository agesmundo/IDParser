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
    chomp $_;
    my $ra_conll = conllrec_to_array($_);
    my $ra_syntadjmat = syntadjmat($ra_conll);
    my $ra_srladjmats = srladjmats($ra_conll);
    next if (isivanproj($ra_srladjmats->[0], $ra_srladjmats->[3])); 
    # $ra_syntadjmat->[3] refers to the array of connected vertices  
    $nrec++;
    # print adjmat_to_string($ra_srladjmats->[0]);
    # print adjmat_to_string($ra_syntadjmat->[0]);
    # print conllarray_to_rowstring($ra_conll);
    print conllarray_to_columnstring($ra_conll);
}

print STDERR "$nrec CoNLL records read\n";
