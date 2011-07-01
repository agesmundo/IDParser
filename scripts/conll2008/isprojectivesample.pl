#!/usr/bin/perl
use warnings;
use strict;
use CoNLL;
use Carp;

my $conllfile = shift;
open CONLL, "< $conllfile" or die "couldnt open file $conllfile: $!\n";

$/ = "";
my $nrec = 0;
my @err;

while (<CONLL>){
    $nrec++;
    chomp $_;
    my $ra_conll = conllrec_to_array($_);
    my $ra_syntadjmat = syntadjmat($ra_conll);
    my $ra_srladjmats = srladjmats($ra_conll);
    if (!isivanproj($ra_srladjmats->[0], $ra_srladjmats->[3])){
	push @err, $nrec;
    }
}

print STDERR "$nrec CoNLL records read\n";
print STDERR join("\t", @err), "\n";
