#!/usr/bin/perl
use warnings;
use strict;
use CoNLL;
use Carp;

my $conllfile = shift;
open CONLL, "< $conllfile" or die "couldnt open file $conllfile: $!\n";
my $rec = shift;

$/ = "";
my $nrec = 0;
my $liftednb = 0; my $rmnb = 0;
my $liftedpb = 0; my $rmpb = 0;
my $srlnb = 0; my $srlpb = 0;
my $nb = 0; my $pb = 0;
my (%lineargovlabel, %linearsrlabel);

while (<CONLL>){
    $nrec++;
    # next if ($nrec != $rec);
    print STDERR $_;
    chomp $_;
    my $ra_conll = conllrec_to_array($_);
    print STDERR conllarray_to_columnstring($ra_conll);
    my $ra_syntadjmat = syntadjmat($ra_conll);
    print STDERR adjmat_to_string($ra_syntadjmat->[0]);
    my $ra_srladjmats = srladjmats($ra_conll);
    print STDERR adjmat_to_string($ra_srladjmats->[0]);
    
    $pb += @{$ra_srladjmats->[4]}; # PropBank predicates
    $nb += @{$ra_srladjmats->[5]}; # NomBank predicates
    $srlpb += @{$ra_srladjmats->[6]}; # PropBank srl arguments
    $srlnb += @{$ra_srladjmats->[7]}; # NomBank srl arguments
    srlstruct($ra_srladjmats->[6], $ra_srladjmats->[7], $ra_syntadjmat->[2]); # syntactic adjacency list 
    my $ra_lifted = 
	projectivize($ra_srladjmats->[0], $ra_srladjmats->[3], $ra_srladjmats->[6], 
		     $ra_srladjmats->[7], $ra_conll, \%lineargovlabel, \%linearsrlabel, 0); 
    # srladjmat, connectivity, propbank, nombank, conll, %liftedlabels, debug
    $liftednb += $ra_lifted->[0]; $rmnb += $ra_lifted->[1];
    $liftedpb += $ra_lifted->[2]; $rmpb += $ra_lifted->[3];
    my $ra_srlconll = projsrladjmat_to_conll($ra_srladjmats->[0], $ra_conll);
    my $ra_projconll = setprojconll($ra_conll, $ra_srlconll);
    print conllarray_to_columnstring($ra_projconll);
}

print STDERR "$nrec CoNLL records read\n";
print STDERR "NomBank preds: $nb NomBank args:  $srlnb lifted args: $liftednb removed args: $rmnb\n";
print STDERR "PropBank preds: $pb PropBank args: $srlpb lifted args: $liftedpb removed args: $rmpb\n";
print STDERR "Linear governor labels:\n";
foreach my $k (sort { $lineargovlabel{$b} <=> $lineargovlabel{$a} } keys %lineargovlabel){
    print STDERR $k, " $lineargovlabel{$k}\n";
}
print STDERR "Linear SRLabels:\n";
foreach my $k (sort { $linearsrlabel{$b} <=> $linearsrlabel{$a} } keys %linearsrlabel){
    print STDERR $k, " $linearsrlabel{$k}\n";
}
