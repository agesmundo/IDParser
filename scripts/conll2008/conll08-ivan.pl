#!/usr/bin/perl
use warnings;
use strict;
use DepGraph;
use Carp;

$/ = "";
my $nrec = 0;

my $projrec = 0;

while (<>){
    $nrec++;
    chomp;
    my $ra_columns = stdin_to_columns($_);
    my $ra_syntadjmat  = synt_adjmat(@{$ra_columns->[0]}-1, $ra_columns);
    my $ra_syntclosure = closure(@{$ra_columns->[0]}-1, $ra_syntadjmat);
    my $ra_syntproj = projmat($ra_syntadjmat, $ra_syntclosure->[0]);
    my $ra_srladjmats  = srl_adjmats(@{$ra_columns->[0]}-1, $ra_columns);
    my $ra_srlclosure = closure(@{$ra_columns->[0]}-1, $ra_srladjmats->[0]);
    my $ra_srlproj = srlcrossprojmat($ra_srladjmats->[0], $ra_srlclosure->[0]);
    if (($ra_syntproj->[2] == 0) && ($ra_srlproj->[2] == 0)){ # projective structures
	print $_, "\n\n";
	++$projrec;
	last if ($projrec > 3000);
    }
}

print STDERR "$nrec CoNLL records read\n\n";
print STDERR "$projrec projective CoNLL record sampled\n\n";
