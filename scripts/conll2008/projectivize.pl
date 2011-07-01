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
# while (defined(my $_conll = <CONLL>)){
    $nrec++;

    # last if ($nrec > 10000);
    next if ($nrec != $rec);

    chomp $_;
    my $ra_conll = conllrec_to_array($_);
    my $ra_syntadjmat = syntadjmat($ra_conll);

    # print STDERR conllarray_to_srlstring($ra_conll);
    
    # my $ra_syntdepth = syntdepth($ra_syntadjmat->[2]); # $ra_syntadjmat->[2] is a ref to the adjacency list

    my $ra_srladjmats = srladjmats($ra_conll);
    $pb += @{$ra_srladjmats->[4]};
    $nb += @{$ra_srladjmats->[5]}; 
    $srlpb += @{$ra_srladjmats->[6]};
    $srlnb += @{$ra_srladjmats->[7]}; 
     
    # srladjmats returns [\@srl_adjmat, \@pb_adjmat, \@nb_adjmat, \@connv, \@pb_preds, \@nb_preds, \@pbsrls, \@nbsrls] 
    # print conllarray_to_columnstring($ra_conll);
    # print STDERR "$nrec:\n";
    # print STDERR adjmat_to_string($ra_srladjmats->[0]);
    # print STDERR join(" ", @{$ra_srladjmats->[3]}), "\n";
    # print "PropBank:\n";

    if (0){
	foreach my $srl (@{$ra_srladjmats->[6]}){
	    print join(" ", @{$srl}), "\n";
	}
	print "NomBank:\n";
	foreach my $srl (@{$ra_srladjmats->[7]}){
	    print join(" ", @{$srl}), "\n";
	}
	
	srlstruct($ra_srladjmats->[6], $ra_srladjmats->[7], $ra_syntadjmat->[2]);
	foreach my $nb (@{$ra_srladjmats->[7]}){
	    print "< $nb->[0] $nb->[1] $nb->[2] $nb->[3] $nb->[4] $nb->[5]\n";
	}
	foreach my $pb (@{$ra_srladjmats->[6]}){
	    print "> $pb->[0] $pb->[1] $pb->[2] $pb->[3] $pb->[4] $pb->[5]\n";
	}
    }

    srlstruct($ra_srladjmats->[6], $ra_srladjmats->[7], $ra_syntadjmat->[2]); 
    # propbank adjmat, nombank adjmat, adjacency list
    my $ra_lifted = 
	projectivize($ra_srladjmats->[0], $ra_srladjmats->[3], $ra_srladjmats->[6], $ra_srladjmats->[7], $ra_conll, \%lineargovlabel, \%linearsrlabel, 0); # srladjmat, connectivity, propbank, nombank, conll, %liftedlabels, debug

    

    $liftednb += $ra_lifted->[0]; $rmnb += $ra_lifted->[1];
    $liftedpb += $ra_lifted->[2]; $rmpb += $ra_lifted->[3];

    # $ra_srladjmats->[7] and ->[6] store refs to (arg index, pred index, semantic role)
    
    # print STDERR adjmat_to_string($ra_srladjmats->[0]);
    # print STDERR join(" ", @{$ra_srladjmats->[3]}), "\n";
    # print STDERR "\n";

    # print STDERR adjmat_to_string($ra_srladjmats->[0]);
    # print STDERR join(" ", @{$ra_conll->[10]}), "\n";
    my $ra_srlconll = projsrladjmat_to_conll($ra_srladjmats->[0], $ra_conll);
    my $ra_projconll = setprojconll($ra_conll, $ra_srlconll);
    # foreach my $ra (@{$ra_projconll}){
	# print STDERR "^ ", join(" ", @{$ra}), "\n";
    # }
    # foreach my $ra (@{$ra_srlconll}){
	# print STDERR "# ", join(" ", @{$ra}), "\n";
    # }
    print conllarray_to_columnstring($ra_projconll);
    # print STDERR conllarray_to_srlstring($ra_projconll);
    # print STDERR conllarray_to_arraystring($ra_projconll);
    # if (!isivanproj($ra_srladjmats->[0], $ra_srladjmats->[3])){
	# print STDERR "ERROR $nrec\n";
    # }
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
