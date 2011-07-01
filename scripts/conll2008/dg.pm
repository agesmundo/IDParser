#!/usr/bin/perl 
use strict; use warnings;

my @adjmat;
for my $i (0..3){
    for my $j (0..3){
	$adjmat[$i][$j] = 0;
   }
}
$adjmat[0][1] = 1;
$adjmat[1][2] = 1;
$adjmat[2][3] = 1;

# print stdout_adjmat(\@adjmat), "\n";

my @rtclosure;
warshall(\@adjmat, \@rtclosure);
print stdout_adjmat(\@rtclosure), "\n";

warshall_v2(\@adjmat);
print stdout_adjmat(\@adjmat), "\n";

print stdout_adjmat(\@adjmat), "\n";
my $ra_rstar = rstar_closure(\@adjmat);
print stdout_adjmat(\@adjmat), "\n";
print stdout_adjmat($ra_rstar), "\n";

sub stdout_adjmat{
    my $ra_adjmat = shift;
    my $l = @{$ra_adjmat}-1;
    my $s;
    for my $i (0..$l){
	for my $j (0..$l){
	    $s .= " [$i, $j] " . $ra_adjmat->[$i][$j];
	}
	$s .= "\n";
    }
    return $s;
}
sub warshall {
    my ($ra_adjmat, $ra_rtclosure) = @_;
    my $n = @{$ra_adjmat}-1;
    for my $g (0..$n){
	for my $d (0..$n){
	    $ra_rtclosure->[$g][$d] = $ra_adjmat->[$g][$d];
	}
    }
    for my $k (0..$n){
	for my $g (0..$n){
	    for my $d (0..$n){
		$ra_rtclosure->[$g][$d] = 
		    $ra_rtclosure->[$g][$d] || 
		    ($ra_rtclosure->[$g][$k] && $ra_rtclosure->[$k][$d]);
	    }
	}
    }
}
sub warshall_v2 {
    my ($ra_adjmat) = @_;
    my $n = @{$ra_adjmat}-1;
    for my $k (0..$n){
	for my $g (0..$n){
	    for my $d (0..$n){
		$ra_adjmat->[$g][$d] = 
		    $ra_adjmat->[$g][$d] || 
		    ($ra_adjmat->[$g][$k] && $ra_adjmat->[$k][$d]);
	    }
	}
    }
}

sub refl_trans_closure{
    my ($n, @ra_adjmat);
    for my $i (1..$n){
	for my $j (1..$n){
	    if ($i == $j or $ra_adjmat[$i][$j]){
		# $ra_closure[$i][$j] = 1;
	    }else{
		# $ra_closure[$i][$j] = 0;
	    }
	}
    }
    for my $k (1..$n){
	for my $i (1..$n){
	    for my $j (1..$n){
		# $ra_closure[$i][$j] = 
		    # $ra_closure[$i][$j] or 
		    # ($ra_closure[$i][$k] and $ra_closure[$k][$j]);
	    }
	}
    }		
}

sub rstar_closure {
    my $ra_adjmat = shift;
    my @adjmat = @{$ra_adjmat};
    my $n = @{$ra_adjmat}-1;
    for my $k (0..$n){
	for my $i (0..$n){
	    for my $j (0..$n){
		$adjmat[$i][$j] = 
		    $adjmat[$i][$j] || ($i == $j) || 
		    ($adjmat[$i][$k] && $adjmat[$k][$j]);
	    }
	}
    }
    return \@adjmat;
}
