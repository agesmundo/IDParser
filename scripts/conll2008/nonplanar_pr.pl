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
my %h_depgold = ();
my %h_dep = ();
my %h_srlgold = ();
my %h_srl = ();
my %h_npdepgold = ();
my %h_npdep = ();
my %h_npsrlgold = ();
my %h_npsrl = ();

my %h_unpdepgold = ();
my %h_unpdep = ();
my %h_unpsrlgold = ();
my %h_unpsrl = ();


while (defined(my $_gold = <GOLD>) && defined(my $_conll = <CONLL>)){
    $nrec++;
    chomp $_gold; chomp $_conll;
    my $ra_conll = conllrec_to_array($_conll);
    my $ra_conllSUrm = conllarray_to_conllarraySUrm($ra_conll);
    my $ra_gold = conllrec_to_array($_gold);
    my $ra_goldSUrm = conllarray_to_conllarraySUrm($ra_gold);

    my $ra_mat = depsrl_adjmats($ra_conllSUrm);
    my $ra_depmat = $ra_mat->[0];
    my $ra_srlmat = $ra_mat->[1];
    my $ra_goldmat = depsrl_adjmats($ra_goldSUrm);
    my $ra_golddepmat = $ra_goldmat->[0];
    my $ra_goldsrlmat = $ra_goldmat->[1];

    my $dep = _mat_to_string($ra_depmat);
    print  "\# $nrec\n", $dep;

    my $ra_goldnp = nonplanarminsubg($ra_golddepmat, $nrec);
    include(\%h_depgold, $ra_goldnp->[2]);
    include(\%h_npdepgold, $ra_goldnp->[0]);
    include(\%h_unpdepgold, $ra_goldnp->[1]);

    my $ra_np = nonplanarminsubg($ra_depmat, $nrec);
    include(\%h_dep, $ra_np->[2]);
    include(\%h_npdep, $ra_np->[0]);
    include(\%h_unpdep, $ra_np->[1]);

    print  _hnp_to_string($ra_np->[0], $nrec, "dep"), "\n";
    print _hlink_to_string($ra_np->[2], $nrec, "dep"), "\n";

    my $srl = _mat_to_string($ra_srlmat);
    print  "\# $nrec\n", $srl;

    $ra_goldnp = nonplanarminsubg($ra_goldsrlmat, $nrec);
    include(\%h_srlgold, $ra_goldnp->[2]);
    include(\%h_npsrlgold, $ra_goldnp->[0]);
    include(\%h_unpsrlgold, $ra_goldnp->[1]);

    $ra_np = nonplanarminsubg($ra_srlmat, $nrec);
    include(\%h_srl, $ra_np->[2]);
    include(\%h_npsrl, $ra_np->[0]);
    include(\%h_unpsrl, $ra_np->[1]);

    print  _hnp_to_string($ra_np->[0], $nrec, "srl"), "\n";
    print _hlink_to_string($ra_np->[2], $nrec, "srl"), "\n";
}


print "$nrec CoNLL records read\n";

my $rh_depintersection = intersection(\%h_depgold, \%h_dep);
print _prf1string("SX", scalar(keys %h_depgold), scalar(keys %h_dep), scalar(keys %$rh_depintersection)), "\n";
my $rh_srlintersection = intersection(\%h_srlgold, \%h_srl);
print _prf1string("SS", scalar(keys %h_srlgold), scalar(keys %h_srl), scalar(keys %$rh_srlintersection)), "\n";

my $rh_npdepintersection = intersection(\%h_npdepgold, \%h_npdep);
print _prf1string("NPSX", scalar(keys %h_npdepgold), scalar(keys %h_npdep), scalar(keys %$rh_npdepintersection)), "\n";
my $rh_unpdepintersection = intersection(\%h_unpdepgold, \%h_unpdep);
print _prf1string("uNPSX", scalar(keys %h_unpdepgold), scalar(keys %h_unpdep), scalar(keys %$rh_unpdepintersection)), "\n";
my $rh_npsrlintersection = intersection(\%h_npsrlgold, \%h_npsrl);
print _prf1string("NPSS", scalar(keys %h_npsrlgold), scalar(keys %h_npsrl), scalar(keys %$rh_npsrlintersection)), "\n";
my $rh_unpsrlintersection = intersection(\%h_unpsrlgold, \%h_unpsrl);
print _prf1string("uNPSS", scalar(keys %h_unpsrlgold), scalar(keys %h_unpsrl), scalar(keys %$rh_unpsrlintersection)), "\n";

sub _prf1string {
    my ($prefix, $goldcard, $card, $intercard) = @_;
    my ($p, $r, $f1, $s);
    $r = ($goldcard > 0) ? ($intercard * 100 / $goldcard) : undef;
    $p = ($card > 0) ? ($intercard * 100 / $card) : undef;
    if (defined($p) and defined($r) and ($p or $r)){
	$f1 = 2 * $p * $r / ($p + $r);
	$s .= sprintf ("%s P %d / %d * 100 = %.2f %%\n", $prefix, $intercard, $card, $p);
	$s .= sprintf ("%s R %d / %d * 100 = %.2f %%\n", $prefix, $intercard, $goldcard, $r);
	$s .= sprintf ("%s F1 %.2f %%\n", $prefix, $f1);
    }elsif (defined($r)){
	# $f1 = undef;
	# $p = undef;
	$s .= sprintf ("%s P %d / %d * 100 = undef\n", $prefix, $intercard, $card);
	$s .= sprintf ("%s R %d / %d * 100 = %.2f %%\n", $prefix, $intercard, $goldcard, $r);
	$s .= sprintf ("%s F1 undef\n", $prefix);
    }else{
	# $f1 = undef;
	# $p = undef;
	# $r = undef;
	$s .= sprintf ("%s P %d / %d * 100 = undef\n", $prefix, $intercard, $card);
	$s .= sprintf ("%s R %d / %d * 100 = undef\n", $prefix, $intercard, $goldcard);
	$s .= sprintf ("%s F1 undef\n", $prefix);
    }
    return $s;
}

sub _hlink_to_string {
    my ($rh, $nrec, $g) = @_;
    my $s = "\# $nrec links $g:\n";
    return $s . "{}\n" if (!keys %$rh);
    $s .= "{\n";
    foreach my $k (sort {$a cmp $b} keys %{$rh}){
	$s .= $k . ':' . $rh->{$k} . "\n";
	confess "-1 $nrec $k $rh->{$k}" if ($rh->{$k} != 1 and $rh->{$k} != -1);
    }
    return $s . "}\n";
}

sub _hnp_to_string {
    my ($rh, $nrec, $g) = @_;
    my $s = "\# $nrec non-planar $g:\n";
    return $s . "{}\n" if (!keys %$rh);
    $s .= "{\n";
    foreach my $k (sort {$a cmp $b} keys %{$rh}){
	$s .= $k . "\n";
	confess "-1 $nrec" if ($rh->{$k} != 2);
    }
    return $s . "}\n";
}

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
