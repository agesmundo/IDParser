#!/usr/bin/perl
use warnings;
use strict;
use DepGraph;
use Carp;

$/ = "";
my $nrec = 0;

my $proj = 0; my $nonproj = 0; 
my $srl_proj = 0; my $srl_nonproj = 0; 
my $propb_proj = 0; my $propb_nonproj = 0; 
my $nomb_proj = 0; my $nomb_nonproj = 0; 

my $token_srlargc = 0; my $srl_argc = 0; my $srl_predc = 0;
my $token_propbargc = 0; my $propb_argc = 0; my $propb_predc = 0;
my $token_nombargc = 0; my $nomb_argc = 0; my $nomb_predc = 0;

my $hpropbc = 0; my $propbclosurec = 0; my $hnombc = 0; my $nombclosurec = 0;

my $loopy = 0; my $cyclic = 0;
my $srl_loopy = 0; my $srl_cyclic = 0;
my $propb_loopy = 0; my $propb_cyclic = 0;
my $nomb_loopy = 0; my $nomb_cyclic = 0;
my $srlc = 0; my $nombc = 0; my $propbc = 0;

while (<>){
    # next if (rand() > 0.04);
    $nrec++;
    next if ($nrec != 3);
    chomp;
    my $ra_columns = stdin_to_columns($_);
    my $ra_syntadjmat  = synt_adjmat(@{$ra_columns->[0]}-1, $ra_columns);
    my $ra_syntclosure = closure(@{$ra_columns->[0]}-1, $ra_syntadjmat);
    my $ra_syntproj = projmat($ra_syntadjmat, $ra_syntclosure->[0]);
    $proj += $ra_syntproj->[1]; $nonproj += $ra_syntproj->[2];
    $loopy += $ra_syntproj->[3]; $cyclic += $ra_syntproj->[4];
    
    my $ra_srladjmats  = srl_adjmats(@{$ra_columns->[0]}-1, $ra_columns);
    $nomb_predc += $ra_srladjmats->[4]; $propb_predc += $ra_srladjmats->[3];
    $srl_predc += $ra_srladjmats->[3] + $ra_srladjmats->[4];
    
    my $ra_srlclosure = closure(@{$ra_columns->[0]}-1, $ra_srladjmats->[0]);
    print STDERR closure_to_stdout($ra_srlclosure->[0]), "\n";
    $srl_argc += $ra_srlclosure->[1]; $token_srlargc += $ra_srlclosure->[2];
    my $ra_srlproj = srlprojmat($ra_srladjmats->[0], $ra_srlclosure->[0]);
    # print STDERR adjmat_to_stdout(@{$ra_srlproj}+0, $ra_srlproj->[0]), "\n";
    $srl_proj += $ra_srlproj->[1]; $srl_nonproj += $ra_srlproj->[2];
    $srl_loopy += $ra_srlproj->[3]; $srl_cyclic += $ra_srlproj->[4];
    $srlc += $ra_srlproj->[5];

    my $ra_propbclosure = closure(@{$ra_columns->[0]}-1, $ra_srladjmats->[1]);
    $propb_argc += $ra_propbclosure->[1]; $token_propbargc += $ra_propbclosure->[2];
    my $ra_propbproj = srlprojmat($ra_srladjmats->[1], $ra_propbclosure->[0]);
    
    $propb_proj += $ra_propbproj->[1]; $propb_nonproj += $ra_propbproj->[2];
    $propb_loopy += $ra_propbproj->[3]; $propb_cyclic += $ra_propbproj->[4];
    $propbc += $ra_propbproj->[5];

    my $ra_nombclosure = closure(@{$ra_columns->[0]}-1, $ra_srladjmats->[2]);
    $nomb_argc += $ra_nombclosure->[1]; $token_nombargc += $ra_nombclosure->[2];
    my $ra_nombproj = srlprojmat($ra_srladjmats->[2], $ra_nombclosure->[0]);
    $nomb_proj += $ra_nombproj->[1]; $nomb_nonproj += $ra_nombproj->[2];
    $nomb_loopy += $ra_nombproj->[3]; $nomb_cyclic += $ra_nombproj->[4];
    $nombc += $ra_nombproj->[5];

    # croak "$nrec $ra_nombproj->[3] $ra_srlproj->[3]" 
	# if (($ra_nombproj->[3] != $ra_srlproj->[3]) && (@{$ra_columns->[0]}-1 < 13));

    # croak "p $nrec $ra_nombproj->[1] + $ra_propbproj->[1] < $ra_srlproj->[1]" 
	# if (($ra_nombproj->[1] + $ra_propbproj->[1]) < $ra_srlproj->[1]);
    # croak "np $nrec $ra_nombproj->[2] + $ra_propbproj->[2] > $ra_srlproj->[2]" 
	# if (($ra_nombproj->[2] + $ra_propbproj->[2]) > $ra_srlproj->[2]);

    my $ra_srlclosurec = srl_inclosurec($ra_syntclosure->[0], $ra_srladjmats->[1], $ra_srladjmats->[2]);
    $hpropbc += $ra_srlclosurec->[0]; $propbclosurec += $ra_srlclosurec->[1]; 
    $hnombc += $ra_srlclosurec->[2]; $nombclosurec += $ra_srlclosurec->[3];
    
    
    
    # croak "$nrec $ra_srlclosurec->[1] $ra_srlclosurec->[3] $ra_srlclosurec->[0] $ra_srlclosurec->[2] "
	#if ((@{$ra_columns->[0]}-1 < 13) && ($ra_srlclosurec->[1] + $ra_srlclosurec->[3]) < ($ra_srlclosurec->[0] + $ra_srlclosurec->[2] - 2));
}

print "$nrec CoNLL records read\n\n";
print "$nrec syntactic structures:";
printf "\t%d loops (%.2f%%), %d cycles (%.2f%%)\n", $loopy, ($loopy*100)/$nrec, $cyclic, ($cyclic*100)/$nrec;
print "$srlc SRL structures:";
printf "\t%d loops (%.2f%%), %d cycles (%.2f%%)\n", $srl_loopy, ($srl_loopy*100)/$srlc, $srl_cyclic, ($srl_cyclic*100)/$srlc;
print "$propbc PropB structures:";
printf "\t%d loops (%.2f%%), %d cycles (%.2f%%)\n", $propb_loopy, ($propb_loopy*100)/$propbc, $propb_cyclic, ($propb_cyclic*100)/$propbc;
print "$nombc NomB structures:";
printf "\t%d loops (%.2f%%), %d cycles (%.2f%%)\n\n", $nomb_loopy, ($nomb_loopy*100)/$nombc, $nomb_cyclic, ($nomb_cyclic*100)/$nombc;

printf "Syntactic PROJECTIVITY  %d projective, %d non-projective: %.2f%%\n", $proj, $nonproj, ($proj*100)/($proj+$nonproj);
printf "SRL       PROJECTIVITY  %d projective, %d non-projective: %.2f%%\n", $srl_proj, $srl_nonproj, ($srl_proj*100)/($srl_proj+$srl_nonproj);
printf "PropBank  PROJECTIVITY  %d projective, %d non-projective: %.2f%%\n", $propb_proj, $propb_nonproj, ($propb_proj*100)/($propb_proj+$propb_nonproj);
printf "NomBank   PROJECTIVITY  %d projective, %d non-projective: %.2f%%\n\n", $nomb_proj, $nomb_nonproj, ($nomb_proj*100)/($nomb_proj+$nomb_nonproj);


printf "SRL      DENSITY  %d arguments, %d predicates, %d tokens: %.2f\n", $srl_argc, $srl_predc, $token_srlargc, $srl_argc/$token_srlargc;
printf "PropBank DENSITY  %d arguments, %d predicates, %d tokens: %.2f\n", $propb_argc, $propb_predc, $token_propbargc, $propb_argc/$token_propbargc;
printf "NomBank  DENSITY  %d arguments, %d predicates, %d tokens: %.2f\n\n", $nomb_argc, $nomb_predc, $token_nombargc, $nomb_argc/$token_nombargc;


printf "PropBank REACHABILITY  %d arguments, %d in the closure of their governor: %.2f%%\n", $hpropbc, $propbclosurec, ($propbclosurec*100)/($hpropbc);
printf "NomBank  REACHABILITY  %d arguments, %d in the closure of their governor: %.2f%%\n\n", $hnombc, $nombclosurec, ($nombclosurec*100)/($hnombc);
