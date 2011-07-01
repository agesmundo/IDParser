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
    my $ra_conll = conllrec_to_array($_);
    my $ra_syntadjmat = syntadjmat($ra_conll);
    my $ra_closure = closure($ra_syntadjmat->[0]);
    print closure_to_string($ra_closure);
}

print STDERR "$nrec CoNLL records read\n";
