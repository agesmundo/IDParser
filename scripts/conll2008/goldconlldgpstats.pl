#!/usr/bin/perl
use warnings;
use strict;
use CoNLL;
use Carp;

my $conllfile = shift;
open CONLL, "< $conllfile" or die "couldnt open file $conllfile: $!\n";

my $rec = shift;
my $debug = 0;
$debug = shift;

my $nrec = 0;
my @a_out;
my @projective; my @nonprojective; my @planar; my @nonplanar; my @nonplandegree; my @edgedegree;
my @derived;

my $edge = 0;
my $nonprojedge = 0;
my $nonplanedge = 0;

{
    local $/ = '';
    while (defined(my $conllrec = <CONLL>)){
	$nrec++;
	if ($rec){
	    last if ($nrec > $rec);
	    next if ($nrec != $rec);
	}
	print STDERR $conllrec if ($debug);
	chomp $conllrec; 
	my $ra_conll = conllrec_to_array($conllrec);
	my $ra_cdg = csyntsemdg($ra_conll);
	print STDERR closure_to_string($ra_cdg->[0]), "\n" if ($debug);
	my $ra_cdgclosure = csyntsemdgclosure($ra_cdg->[0]);
	print STDERR closure_to_string($ra_cdgclosure), "\n" if ($debug);
	my @a;
	push @a, $nrec;
	my $ra_cproj = is_cprojective($ra_cdg->[0], $ra_cdgclosure, $ra_cdg->[1], $ra_cdg->[2]);
	$edge += $ra_cproj->[0];
	if (!$ra_cproj->[1]){
	    push @a, 1;
	    push @a, 1;
	    push @a, 0;
	    
	    push @projective, $nrec; push @planar, $nrec; push @{$nonplandegree[0]}, $nrec;
	}else{
	    $nonprojedge += $ra_cproj->[1];
	    push @a, 0;
	    
	    push @nonprojective, $nrec;
	    my $ra_cplan = is_cplanar($ra_cdg->[0]);
	    if (!$ra_cplan->[1]){
		push @a, 1;
		push @a, 0;
		
		push @planar, $nrec; push @{$nonplandegree[0]}, $nrec;
	    }else{
		$nonplanedge += $ra_cplan->[1];
		push @a, 0;
		my $degree = 0;
		$degree = cplanaritydegree($ra_cdg->[0], $ra_cdg->[1], $ra_cdg->[2], \@edgedegree, $debug);
		# print STDERR join(" ", @{$ra_cdg->[1]}), "\n";
		# print STDERR join(" ", @{$ra_cdg->[2]}), "\n";
		push @a, $degree;
		
		push @nonplanar, $nrec; push @{$nonplandegree[$degree]}, $nrec;
	    }
	}
	push @a_out, \@a;
	# last;
    }
}

print STDERR "STATS ON THE UNION OF SYNT AND SEM GRAPHS, MULTI-GRAPHS ARE REDUCED TO GRAPHS\n\n";
printf STDERR ("NONPROJECTIVE EDGE : %d, %d, %-3.2f\n", $nonprojedge, $edge, $nonprojedge/$edge*100);
printf STDERR ("NONPLANAR EDGE : %d, %d, %-3.2f\n", $nonplanedge, $edge, $nonplanedge/$edge*100);

printf STDERR ("PROJECTIVE 1/0: %d, %d, %d, %-3.2f\n", 0+@projective, 0+@nonprojective, 0+@projective+@nonprojective, (0+@projective)/(0+@projective+@nonprojective)*100);

printf STDERR ("PLANAR 1/0: %d, %d, %d, %-3.2f\n", 0+@planar, 0+@nonplanar, 0+@planar+@nonplanar, (0+@planar)/(0+@planar+@nonplanar)*100);

foreach my $deg (0..@edgedegree-1){
    next if (!defined($edgedegree[$deg]));
    printf STDERR ("NONPLANARITY EDGE DEGREE %d: %d\n", $deg, $edgedegree[$deg]);
}

foreach my $deg (0..@nonplandegree-1){
    next if (!defined($nonplandegree[$deg]) || ref($nonplandegree[$deg]) ne 'ARRAY');
    printf STDERR ("NONPLANARITY DEGREE %d: %d\n", $deg, @{$nonplandegree[$deg]}+0);
}


print STDERR "$nrec CoNLL rec done\n\n";

