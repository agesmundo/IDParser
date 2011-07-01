#!/usr/bin/perl
use warnings;
use strict;
use CoNLL;
use Carp;

my $goldfile = shift;
open GOLD, "< $goldfile" or die "couldnt open file $goldfile: $!\n";
my $modelfile = shift;
open MODEL, "< $modelfile" or die "couldnt open file $modelfile: $!\n";

$/ = "";
my $nrec = 0; my $rec = shift;
my (%confmatrix, %h_srls);

while ((defined(my $goldrec = <GOLD>)) && (defined(my $modelrec = <MODEL>))){
    $nrec++;
    next if ($rec && ($nrec != $rec));
    last if ($rec && ($nrec > $rec));
    chomp $goldrec; 
    chomp $modelrec;
    next if (($goldrec =~ /^\s*$/) || ($modelrec =~ /^\s*$/));
    my $ra_gold = conllrec_to_array($goldrec);
    my $ra_model = conllrec_to_array($modelrec);
    
    my $ra_gold_srladjmats = srl_adjmats($ra_gold);
    # print STDERR adjmat_to_string($ra_gold_srladjmats->[0]);
    my $ra_model_srladjmats = srl_adjmats($ra_model);
    # print STDERR adjmat_to_string($ra_model_srladjmats->[0]);

    srl_confusionm($ra_gold_srladjmats->[0], $ra_model_srladjmats->[0], \%confmatrix, \%h_srls);
}
print STDERR "$nrec CoNLL rec read\n\n";
print STDERR confm_to_string(\%confmatrix);
print STDERR confm_to_f1_string(\%confmatrix);
