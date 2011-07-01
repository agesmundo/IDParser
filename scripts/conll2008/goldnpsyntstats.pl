#!/usr/bin/perl
use warnings;
use strict;
use CoNLL;
use Carp;

$/ = "";
my $nrec = 0;
my $conllrec;
my $proj = 0; my $nonproj = 0; my $edge = 0;

while (defined(my $conllrec = <>)){
    $nrec++;
    chomp $conllrec;
    my $ra_conll = conllrec_to_array($conllrec);
    my $ra_syntadjmat  = syntadjmat($ra_conll); # return [\@adjmat, \@connv, \@adjlist];
    my $ra_syntclosure = closure($ra_syntadjmat->[0]);
    my $ra_syntproj = marcusprojmat($ra_syntadjmat->[0], $ra_syntclosure, $ra_syntadjmat->[1]);

    $nonproj += @{$ra_syntproj->[1]}; $proj += @{$ra_syntproj->[2]};
    $edge += @{$ra_syntproj->[1]} + @{$ra_syntproj->[2]};
}

print STDERR "$nrec CoNLL rec done\n";
printf STDERR ("NP EDGE 1/0: %d, %d, %-3.2f\n", $proj, $nonproj, $proj/$edge*100);
