#!/usr/bin/perl

use warnings; use strict;

use CoNLL;
use Carp;

my $goldfile = shift;
open GOLD, "< $goldfile" or die "couldnt open file $goldfile: $!\n";
my $conllfile = shift;
open CONLL, "< $conllfile" or die "couldnt open file $conllfile: $!\n";

$/ = "";
my $nrec = 0;
my $p = 0;
my $r = 0;
my $t = 0;

print "# SSCR EVALB  ==================================================================\n";

while (defined(my $_gold = <GOLD>) && defined(my $_conll = <CONLL>)){
    $nrec++;
    chomp $_gold; chomp $_conll;
    my $ra_gold = conllrec_to_array($_gold);
    my $ra_goldSUrm = conllarray_to_conllarraySUrm($ra_gold);
    my $ra_conll = conllrec_to_array($_conll);
    my $ra_conllSUrm = conllarray_to_conllarraySUrm($ra_conll);

    my $ra_mat = depsrl_adjmats($ra_conllSUrm);
    my $ra_srlmat = $ra_mat->[1];
    my $ra_goldmat = depsrl_adjmats($ra_goldSUrm);
    my $ra_goldsrlmat = $ra_goldmat->[1];
    
    my $ra_npgold = nonplanarminsubg($ra_goldsrlmat, $nrec);
    # include(\%h_npsrlgold, $ra_npgold->[0]);
    my $ra_np = nonplanarminsubg($ra_srlmat, $nrec);
    # include(\%h_npsrl, $ra_np->[0]);
    my $rh_npinter = intersection($ra_npgold->[0], $ra_np->[0]);

    if (@{$ra_conll->[$CoNLL::conllf{'ID'}]} > 0) {
	my ($recall,$precision) = ('NaN', 'NaN');
	if (scalar(keys %{$ra_npgold->[0]}) > 0) { 
	    $recall = scalar(keys %{$rh_npinter}) *100.0/scalar(keys %{$ra_npgold->[0]});
	}
	if (scalar(keys %{$ra_np->[0]}) > 0) { 
	    $precision   = scalar(keys %{$rh_npinter}) *100.0/scalar(keys %{$ra_np->[0]});
	}
	printf "  %4d %4d    0  %6.2f %6.2f  %4d    %4d        %4d    0 0 0 0\n", 
	$nrec, @{$ra_conll->[$CoNLL::conllf{'ID'}]}-1, 
	$recall, $precision, 
	scalar(keys %{$rh_npinter}), 
	scalar(keys %{$ra_npgold->[0]}), 
	scalar(keys %{$ra_np->[0]});
    }
    $t += scalar(keys %{$rh_npinter});
    $p += scalar(keys %{$ra_np->[0]});
    $r += scalar(keys %{$ra_npgold->[0]});
}

print STDERR "$t $p $r\n";
print STDERR "$nrec CoNLL records done\n";

sub _mat_to_string {
    my $mat = shift;
    my $dim = @{$mat}-1;
    my $s = '';
    for my $i (0..$dim){
	$s .= "$i:\t";
	for my $j (0..$dim){
	    if (defined($mat->[$i][$j]) && @{$mat->[$i][$j]}){
		$s .= "{" . join(",", @{$mat->[$i][$j]}) . "} ";
	    }else{
		$s .= "$j ";
		# $s .= ($j == 0) ? "$i\t" : "$j ";
	    }
	}
	$s .= "\n";
    }
    return $s . "\n";
}
