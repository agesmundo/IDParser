#!/usr/bin/perl

use strict; use warnings;

use CoNLL;
use Carp;

my $conllfile = shift;
open CONLL, "< $conllfile" or die "couldnt open file $conllfile: $!\n";

$/ = "";
my $nrec = 0;
my $t = 0;
while (defined(my $_conll = <CONLL>)){
    $nrec++;
    chomp $_conll;
    my $ra_conll = conllrec_to_array($_conll);
    my $ra_conllSUrm = conllarray_to_conllarraySUrm($ra_conll);
    my $ra_srl = connectedsrladjmat($ra_conllSUrm);
    # print STDERR "\# $nrec\n", _mat_to_string($ra_srl->[0]), "\n"; 
    if ($ra_srl->[1] == (1+$ra_srl->[2])){
	$t++;
    }
    # $nrec++;
    # last if ($nrec > 7);
}

print STDERR "$nrec CoNLL records done\n";
print STDERR "$t CoNLL trees\n";

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
