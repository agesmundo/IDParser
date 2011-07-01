#!/usr/bin/perl
use warnings;
use strict;
use DepGraph;
use Carp;

$/ = "";
my $nrec = 0;
while (<>){
    $nrec++;
    chomp;
    my $ra_columns = stdin_to_columns($_);
    foreach my $ra_column (@{$ra_columns}){
	column_to_stdout($ra_column); 
    }
    my $ra_syntadjmat  = synt_adjmat(@{$ra_columns->[0]}-1, $ra_columns);
    print "synt adjmat\n";
    adjmat_to_stdout(@{$ra_columns->[0]}-1, $ra_syntadjmat) if (@{$ra_syntadjmat}); # syntactic dependencies
    my $ra_syntclosure = closure(@{$ra_columns->[0]}-1, $ra_syntadjmat);
    print "synt closure\n";
    closure_to_stdout($ra_syntclosure->[0]) if (@{$ra_syntclosure->[0]});
    # print "$ra_syntclosure->[1] $ra_syntclosure->[2] $ra_syntclosure->[3]\n";
    
    my $ra_srladjmats  = srl_adjmats(@{$ra_columns->[0]}-1, $ra_columns);
    my $ra_srlclosure = closure(@{$ra_columns->[0]}-1, $ra_srladjmats->[0]);
    my $ra_propbclosure = closure(@{$ra_columns->[0]}-1, $ra_srladjmats->[1]);
    my $ra_nombclosure = closure(@{$ra_columns->[0]}-1, $ra_srladjmats->[2]);
    
    print "srl adjmat\n";
    adjmat_to_stdout(@{$ra_columns->[0]}-1, $ra_srladjmats->[0]) 
	if (@{$ra_srladjmats->[0]}); # PropBank and NomBank srl adjmat
    print "srl closure\n";
    closure_to_stdout($ra_srlclosure->[0]) if (@{$ra_srlclosure->[0]});
    # print "sharedargc= $ra_srlclosure->[3]\n";
    print "propb adjmat\n";
    adjmat_to_stdout(@{$ra_columns->[0]}-1, $ra_srladjmats->[1]) 
	if (@{$ra_srladjmats->[1]}); # PropBank srl adjmat
    print "propb closure\n";
    closure_to_stdout($ra_propbclosure->[0]) if (@{$ra_propbclosure->[0]});
    # print "sharedargc= $ra_propbclosure->[3]\n";
    print "nomb adjmat\n";
    adjmat_to_stdout(@{$ra_columns->[0]}-1, $ra_srladjmats->[2]) 
	if (@{$ra_srladjmats->[2]}); # NomBank  srl adjmat
    print "nomb closure\n";
    closure_to_stdout($ra_nombclosure->[0]) if (@{$ra_nombclosure->[0]});
    # print "sharedargc= $ra_nombclosure->[3]\n";
    
    my $ra_syntproj = projmat($ra_syntadjmat, $ra_syntclosure->[0]);
    print "synt projectivity\n";
    closure_to_stdout($ra_syntproj->[0]) if (@{$ra_syntproj->[0]});
    # print "$ra_syntproj->[1] $ra_syntproj->[2]\n";
    my $ra_srlproj = projmat($ra_srladjmats->[0], $ra_srlclosure->[0]);
    print "srl projectivity\n";
    closure_to_stdout($ra_srlproj->[0]) if (@{$ra_srlproj->[0]});
    # print "$ra_srlproj->[1] $ra_srlproj->[2]\n";
    my $ra_propbproj = projmat($ra_srladjmats->[1], $ra_propbclosure->[0]);
    print "propb projectivity\n";
    closure_to_stdout($ra_propbproj->[0]) if (@{$ra_propbproj->[0]});
    # print "$ra_propbproj->[1] $ra_propbproj->[2]\n";
    my $ra_nombproj = projmat($ra_srladjmats->[2], $ra_nombclosure->[0]);
    print "nomb projectivity\n";
    closure_to_stdout($ra_nombproj->[0]) if (@{$ra_nombproj->[0]});
    # print "$ra_nombproj->[1] $ra_nombproj->[2]\n";

    $ra_srlproj = srlprojmat($ra_srladjmats->[0], $ra_srlclosure->[0]);
    print "srl C projectivity\n";
    closure_to_stdout($ra_srlproj->[0]) if (@{$ra_srlproj->[0]});
    
    # print "$ra_srlproj->[1] $ra_srlproj->[2]\n";
    $ra_propbproj = srlprojmat($ra_srladjmats->[1], $ra_propbclosure->[0]);
    print "propb C projectivity\n";
    closure_to_stdout($ra_propbproj->[0]) if (@{$ra_propbproj->[0]});
    # print "$ra_propbproj->[1] $ra_propbproj->[2]\n";
    $ra_nombproj = srlprojmat($ra_srladjmats->[2], $ra_nombclosure->[0]);
    print "nomb C projectivity\n";
    closure_to_stdout($ra_nombproj->[0]) if (@{$ra_nombproj->[0]});
    # print "$ra_nombproj->[1] $ra_nombproj->[2]\n";
    my $ra_srlcrossproj = srlcrossprojmat($ra_srladjmats->[0], $ra_srlclosure->[0]);
    print "srl cross projectivity\n";
    closure_to_stdout($ra_srlcrossproj->[0]) if (@{$ra_srlcrossproj->[0]});
    print "$ra_srlcrossproj->[2]\n";
    last;
}

print "$nrec\n";


