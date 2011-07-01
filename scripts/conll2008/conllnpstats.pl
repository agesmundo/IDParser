#!/usr/bin/perl
use warnings;
use strict;
use CoNLL;
use Carp;

my $derivfile = shift;
open DERIV, "< $derivfile" or die "couldnt open file $derivfile: $!\n";
my $conllfile = shift;
open CONLL, "< $conllfile" or die "couldnt open file $conllfile: $!\n";

my $rec = shift;
my $debug = 0;
$debug = shift;

my $nrec = 0;
my @a_out;
my @projective; my @nonprojective; my @planar; my @nonplanar; my @nonplandegree;
my @derived;

while (<DERIV>){
    chomp;
    my @a_fields = split /\s+/, $_;
    my $l = $a_fields[0];
    my $u = $a_fields[$#a_fields];
    if ($u == -1){
	foreach my $i ($l..$a_fields[$#a_fields-2]){
	    $derived[$i] = 1;
	}
	push @derived, 0;
    }else{
	foreach my $i ($l..$u){
	    $derived[$i] = 1;
	}
    }
}
if ($debug){
    foreach my $i (0..@derived-1){
	print STDERR "$i,$derived[$i] ";
	print STDERR "\n" if (!$derived[$i]);
    }
    print STDERR "\n";
}

my ($parsed, $nonparsed);
foreach my $i (@derived){
    if ($i){
	$parsed++;
    }else{
	$nonparsed++;
    }
}

printf STDERR ("DERIVED 1/0: %d, %d, %d, %-3.2f\n", $parsed, $nonparsed, $parsed+$nonparsed, ($parsed)/($parsed+$nonparsed)*100);


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
	my $ra_csemdg = csemdg($ra_conll);
	my $ra_csemdgclosure = csemdgclosure($ra_csemdg->[0]);
	my @a;
	push @a, $nrec;
	if (is_cprojective($ra_csemdg->[0], $ra_csemdgclosure, $ra_csemdg->[1], $ra_csemdg->[2])){
	    push @a, 1;
	    push @a, 1;
	    push @a, 0;
	    
	    push @projective, $nrec; push @planar, $nrec; push @{$nonplandegree[0]}, $nrec;
	}else{
	    push @a, 0;
	    
	    push @nonprojective, $nrec;
	    if (is_cplanar($ra_csemdg->[0], $ra_csemdgclosure)){
		push @a, 1;
		push @a, 0;
		
		push @planar, $nrec; push @{$nonplandegree[0]}, $nrec;
	    }else{
		push @a, 0;
		my $degree = 0;
		$degree = cplanaritydegree($ra_csemdg->[0], $ra_csemdg->[1], $ra_csemdg->[2], $debug);
		# print STDERR join(" ", @{$ra_csemdg->[1]}), "\n";
		# print STDERR join(" ", @{$ra_csemdg->[2]}), "\n";
		push @a, $degree;
		
		push @nonplanar, $nrec; push @{$nonplandegree[$degree]}, $nrec;
	    }
	}
	push @a_out, \@a;
    }
}



printf STDERR ("PROJECTIVE 1/0: %d, %d, %d, %-3.2f\n", 0+@projective, 0+@nonprojective, 0+@projective+@nonprojective, (0+@projective)/(0+@projective+@nonprojective)*100);

printf STDERR ("PLANAR 1/0: %d, %d, %d, %-3.2f\n", 0+@planar, 0+@nonplanar, 0+@planar+@nonplanar, (0+@planar)/(0+@planar+@nonplanar)*100);


my $ra_stats = _stats(\@projective, \@derived);
printf STDERR ("PROJECTIVE DERIVED: %d, %d, %-3.2f\n", $ra_stats->[0], $ra_stats->[1], $ra_stats->[0]/$ra_stats->[1]*100);

$ra_stats = _stats(\@nonprojective, \@derived);
printf STDERR ("NONPROJECTIVE DERIVED: %d, %d, %-3.2f\n", $ra_stats->[0], $ra_stats->[1], $ra_stats->[0]/$ra_stats->[1]*100);
$ra_stats = _stats(\@planar, \@derived);
printf STDERR ("PLANAR DERIVED: %d, %d, %-3.2f\n", $ra_stats->[0], $ra_stats->[1], $ra_stats->[0]/$ra_stats->[1]*100);
$ra_stats = _stats(\@nonplanar, \@derived);
printf STDERR ("NONPLANAR DERIVED: %d, %d, %-3.2f\n", $ra_stats->[0], $ra_stats->[1], $ra_stats->[0]/$ra_stats->[1]*100);


foreach my $deg (0..@nonplandegree-1){
    next if (!defined($nonplandegree[$deg]) || ref($nonplandegree[$deg]) ne 'ARRAY');
    $ra_stats = _stats($nonplandegree[$deg], \@derived);
    printf STDERR ("NONPLANARITY DEGREE %d: %d, %d, %-3.2f\n", $deg, $ra_stats->[0], $ra_stats->[1], $ra_stats->[0]/$ra_stats->[1]*100);
}

print STDERR "$nrec CoNLL rec done\n\n";

sub _printrecn {
    my ($ra, $ra_derived) = @_;
    my $derived = 0;
    foreach my $s (@{$ra}){
	if (!$ra_derived->[$s-1]){
	    print STDERR "\n\n", "DERIV FAIL $s ";
	}
    }
    print STDERR "\n\n";
    return [$derived, 0+@{$ra}];
}
sub _stats {
    my ($ra, $ra_derived) = @_;
    my $derived = 0;
    foreach my $s (@{$ra}){
	$derived++ if ($ra_derived->[$s-1]);
    }
    return [$derived, 0+@{$ra}];
}

