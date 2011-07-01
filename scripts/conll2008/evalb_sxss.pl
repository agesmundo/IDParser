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

print "# MICRO SXSS EVALB  ==================================================================\n";

while (defined(my $_gold = <GOLD>) && defined(my $_conll = <CONLL>)){
    $nrec++;
    chomp $_gold; chomp $_conll;
    my $ra_gold = conllrec_to_array($_gold);
    my $ra_goldSUrm = conllarray_to_conllarraySUrm($ra_gold);
    my $ra_conll = conllrec_to_array($_conll);
    my $ra_conllSUrm = conllarray_to_conllarraySUrm($ra_conll);

    my $ra_ssgoldmat = ss_adjmat($ra_goldSUrm);
    my $ra_ssmat = ss_adjmat($ra_conllSUrm);
    my $ra_sxgoldmat = sx_adjmat($ra_goldSUrm);
    my $ra_sxmat = sx_adjmat($ra_conllSUrm);

    my $ra_sspr = pr($ra_ssgoldmat, $ra_ssmat);

    my $ra_sxpr = pr($ra_sxgoldmat, $ra_sxmat);

    if (@{$ra_conll->[$CoNLL::conllf{'ID'}]} > 0) {
	my ($recall,$precision) = ('NaN', 'NaN');
	if ($ra_sxpr->[2] > 0) { 
	    $recall = ($ra_sxpr->[0]+$ra_sspr->[0])*100.0/($ra_sxpr->[2]+$ra_sspr->[2]);
	}
	if ($ra_sxpr->[1] > 0) { 
	    $precision   = ($ra_sxpr->[0]+$ra_sspr->[0]) *100.0/($ra_sxpr->[1]+$ra_sxpr->[1]);
	}
	printf("  %4d %4d    0  %6.2f %6.2f  %4d    %4d        %4d    0 0 0 0\n", 
	       $nrec, @{$ra_conll->[$CoNLL::conllf{'ID'}]}-1, 
	       $recall, $precision, 
	       ($ra_sxpr->[0]+$ra_sspr->[0]), 
	       ($ra_sxpr->[2]+$ra_sspr->[2]), 
	       ($ra_sxpr->[1]+$ra_sspr->[1]));
    }
    $t += ($ra_sxpr->[0]+$ra_sspr->[0]);
    $p += ($ra_sxpr->[1]+$ra_sspr->[1]);
    $r += ($ra_sxpr->[2]+$ra_sspr->[2]);
	
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
