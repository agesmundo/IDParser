#!/usr/bin/perl
use warnings;
use strict;
use CoNLL;
use Carp;

my $conllfile = shift;
open CONLL, "< $conllfile" or die "couldnt open file $conllfile: $!\n";

$/ = "";
my $nrec = 0;
my $ambisrl = 0; my $unconnsrl = 0;
my (@ambistruct, @unconnstruct);
while (<CONLL>){
    $nrec++;
    chomp;
    my $ra_itjh = itjhrec_to_srladjmat($_);
    my $ra_syntadjmat = syntadjmat($ra_itjh->[0]);
    my $ra_projconll = itjhadjmat_to_conlladjmat($ra_itjh->[0], $ra_syntadjmat->[0], $ra_itjh->[1]);
    $ambisrl += $ra_projconll->[1]; $unconnsrl += $ra_projconll->[2];
    push @ambistruct, $nrec if ($ra_projconll->[1]);
    push @unconnstruct, $nrec if ($ra_projconll->[2]);
    my $ra_itjhprojconll = itjhinvproj_to_array($ra_itjh->[0], $ra_projconll->[0]); 
    print conllarray_to_columnstring($ra_itjhprojconll);
}

print STDERR "$nrec CoNLL records read\n";
print STDERR "$ambisrl $unconnsrl\n";
print STDERR join(" ", @ambistruct), "\n";
print STDERR join(" ", @unconnstruct), "\n";
