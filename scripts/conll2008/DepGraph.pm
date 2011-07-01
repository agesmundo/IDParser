#!/usr/bin/perl

package DepGraph;

use strict;
use warnings;
use Carp;

BEGIN {
    use Exporter   ();
    our ($VERSION, @ISA, @EXPORT, @EXPORT_OK, @EXPORT_FAIL, %EXPORT_TAGS);
    @ISA         = qw(Exporter);
    @EXPORT      = qw(&stdin_to_columns &column_to_stdout &adjmat_to_stdout &closure_to_stdout
		      &srl_inclosurec &synt_adjmat &srl_adjmats &closure &projmat &srlprojmat &srlcrossprojmat
		      &get_idesc &get_ianc &get_desc &get_anc &lca);
}

sub column_to_stdout($) { # prints to stdout the input column
    my $ra_column = shift;
    print join(" ", @{$ra_column}), "\n";
}

sub stdin_to_columns($){ # stores an input CoNLL record in the output array
    my $rec = shift;
    my @rows = split /\n+/, $rec;
    my @columns;
    for my $row (@rows){
	my @fields = split /\s+/, $row;
	my $j;
	map {push @{$columns[$j++]}, $_;} @fields;
    }
    for my $i (0..$#columns){
	unshift @{$columns[$i]}, '-1';
    }
    return \@columns; # 
}


sub srl_inclosurec($$$){ # counts the number of PropBank and NomBank args that belong to the projection of their governor
    my ($ra_closure, $ra_propb, $ra_nomb) = @_;
    my $propbc = 0; my $nombc = 0; my $srlpropbc = 0; my $srlnombc = 0;
    my $dim = @{$ra_closure}-1;
    for my $i (0..$dim){
	for my $j (0..$dim){
	    if (defined($ra_propb->[$i][$j]) && (ref($ra_propb->[$i][$j]) eq 'ARRAY')){ 
		$srlpropbc += @{$ra_propb->[$i][$j]};
		if (($i == $j) || $ra_closure->[$i][$j]){
		    $propbc += @{$ra_propb->[$i][$j]};
		}
	    }
	    if (defined($ra_nomb->[$i][$j]) && (ref($ra_nomb->[$i][$j]) eq 'ARRAY')){ 
		$srlnombc += @{$ra_nomb->[$i][$j]};
		if (($i == $j) || $ra_closure->[$i][$j]){
		    $nombc += @{$ra_nomb->[$i][$j]};
		}
	    }
	}
    }
    return [$srlpropbc, $propbc, $srlnombc, $nombc];
}

sub srlcrossprojmat($$) { # compute the projectivity matrix where projective links gets the value 1, and crossing non-projective -1
    my ($ra_adjmat, $ra_closure) = @_;
    my $dim = @{$ra_closure}-1;
    my (@connected, $p, $np); $p = 0; $np = 0;
    my $loopy = 0; my $cyclic = 0; my $n = 0;
    for my $i (0..$dim){
	$connected[$i] = 0;
	for my $j (0..$i){
	    if ($ra_closure->[$i][$j] || $ra_closure->[$j][$i]){
		$connected[$i] = 1; $connected[$j] = 1;
	    }
	    if ($ra_closure->[$i][$j] && $ra_closure->[$j][$i] && ($i != $j)){
		$cyclic = 1;
	    }
	}
    }
    my @mat;
    for my $i (0..$dim){
	for my $j (0..$dim){
	    if (defined($ra_adjmat->[$i][$j]) && (ref($ra_adjmat->[$i][$j]) eq 'ARRAY')){ 
		$n = 1;
		$loopy = 1 if ($i == $j); 
		$mat[$i][$j] = 1;
		my ($l, $u); if ($i > $j){$u = $i; $l = $j}else{$u = $j; $l = $i};
		for my $k ($l+1..$u-1){
		    if ($connected[$k]){ #  && !($ra_closure->[$i][$k])){
			for my $m (0..$l-1){
			    if (defined($ra_adjmat->[$m][$k]) && (ref($ra_adjmat->[$m][$k]) eq 'ARRAY')){
				$mat[$i][$j] = -1; $np++;last
			    }
			    if (defined($ra_adjmat->[$k][$m]) && (ref($ra_adjmat->[$k][$m]) eq 'ARRAY')){
				$mat[$i][$j] = -1; $np++;last
			    }
			}
			last if ($mat[$i][$j] == -1);
			for my $m ($u+1..$dim){
			    if (defined($ra_adjmat->[$m][$k]) && (ref($ra_adjmat->[$m][$k]) eq 'ARRAY')){
				$mat[$i][$j] = -1; $np++;last
			    }
			    if (defined($ra_adjmat->[$k][$m]) && (ref($ra_adjmat->[$k][$m]) eq 'ARRAY')){
				$mat[$i][$j] = -1; $np++;last
			    }
			}
			last if ($mat[$i][$j] == -1);
		    }
		}
		$p++ if ($mat[$i][$j] > 0);
	    }else{
		$mat[$i][$j] = 0;
	    }
	}
    }
    return [\@mat, $p, $np, $loopy, $cyclic, $n]; # @mat stores the proj matrix, $p the number of projective links, $np the number of non-projective ones
}


sub srlprojmat($$) { # compute the projectivity matrix where projective links gets the value 1, and crossing non-projective -1
    my ($ra_adjmat, $ra_closure) = @_;
    my $dim = @{$ra_closure}-1;
    my (@connected, $p, $np); $p = 0; $np = 0;
    my $loopy = 0; my $cyclic = 0; my $n = 0;
    for my $i (0..$dim){
	$connected[$i] = 0;
	for my $j (0..$i){
	    if ($ra_closure->[$i][$j] || $ra_closure->[$j][$i]){
		$connected[$i] = 1; $connected[$j] = 1;
	    }
	    if ($ra_closure->[$i][$j] && $ra_closure->[$j][$i] && ($i != $j)){
		$cyclic = 1;
	    }
	}
    }
    my @mat;
    for my $i (0..$dim){
	for my $j (0..$dim){
	    if (defined($ra_adjmat->[$i][$j]) && (ref($ra_adjmat->[$i][$j]) eq 'ARRAY')){ 
		$n = 1;
		$loopy = 1 if ($i == $j); 
		$mat[$i][$j] = 1;
		my ($l, $u); if ($i > $j){$u = $i; $l = $j}else{$u = $j; $l = $i};
		for my $k ($l+1..$u-1){
		    print STDERR ">> $i, $j, $k, $connected[$k], $ra_closure->[$i][$k]\n";
		    if ($connected[$k] && !($ra_closure->[$i][$k])){
			$np++; print STDERR "NP $i, $j\n";last;
			for my $m (0..$l-1){
			    if (defined($ra_adjmat->[$m][$k]) && (ref($ra_adjmat->[$m][$k]) eq 'ARRAY')){
				$mat[$i][$j] = -1; $np++;
				print STDERR "NP $i, $j\n";last;
				    
			    }
			    if (defined($ra_adjmat->[$k][$m]) && (ref($ra_adjmat->[$k][$m]) eq 'ARRAY')){
				$mat[$i][$j] = -1; $np++;
				print STDERR "NP $i, $j\n";last;
			    }
			}
			last if ($mat[$i][$j] == -1);
			for my $m ($u+1..$dim){
			    if (defined($ra_adjmat->[$m][$k]) && (ref($ra_adjmat->[$m][$k]) eq 'ARRAY')){
				$mat[$i][$j] = -1; $np++;
				print STDERR "NP $i, $j\n";last;
			    }
			    if (defined($ra_adjmat->[$k][$m]) && (ref($ra_adjmat->[$k][$m]) eq 'ARRAY')){
				$mat[$i][$j] = -1; $np++;
				print STDERR "NP $i, $j\n";last;
			    }
			}
			last if ($mat[$i][$j] == -1);
		    }
		}
		$p++ if ($mat[$i][$j] > 0);
	    }else{
		$mat[$i][$j] = 0;
	    }
	}
    }
    print STDERR "\n\n";
    return [\@mat, $p, $np, $loopy, $cyclic, $n]; # @mat stores the proj matrix, $p the number of projective links, $np the number of non-projective ones
}

sub projmat($$) { # compute the projectivity matrix where projective links gets the value 1, and non-projective -1
    my ($ra_adjmat, $ra_closure) = @_;
    my $dim = @{$ra_closure}-1;
    my (@connected, $p, $np); $p = 0; $np = 0;
    my $loopy = 0; my $cyclic = 0; my $n = 0;
    for my $i (0..$dim){
	$connected[$i] = 0;
	for my $j (0..$i){
	    if ($ra_closure->[$i][$j] || $ra_closure->[$j][$i]){
		$connected[$i] = 1; $connected[$j] = 1;
	    }
	    if ($ra_closure->[$i][$j] && $ra_closure->[$j][$i] && ($i != $j)){
		$cyclic = 1;
	    }
	}
    }
    my @mat;
    for my $i (0..$dim){
	for my $j (0..$dim){
	    if (defined($ra_adjmat->[$i][$j]) && (ref($ra_adjmat->[$i][$j]) eq 'ARRAY')){ 
		$n = 1;
		$loopy = 1 if ($i == $j); 
		$mat[$i][$j] = 1;
		my ($l, $u); if ($i > $j){$u = $i; $l = $j}else{$u = $j; $l = $i};
		for my $k ($l+1..$u-1){
		    if ($connected[$k] && !($ra_closure->[$i][$k])){
			$mat[$i][$j] = -1; $np++;
			print STDERR "NP $i, $j\n";
			last;
		    }
		}
		$p++ if ($mat[$i][$j] > 0);
	    }else{
		$mat[$i][$j] = 0;
	    }
	}
    }
    return [\@mat, $p, $np, $loopy, $cyclic, $n]; # @mat stores the proj matrix, $p the number of projective links, $np the number of non-projective ones
}

sub closure($$){ # computes the transitive closure of a dependency graph
    my ($dim, $ra_adjmat) = @_;
    my @mat;
    my $argc = 0; my $predc = 0; my $tokenc = 0; 
    for my $j (0..$dim){
	my $argcpercol = 0;
	for my $i (0..$dim){
	    # if (($i == $j) || (defined($ra_adjmat->[$i][$j]) && (ref($ra_adjmat->[$i][$j]) eq 'ARRAY'))){
	    if (defined($ra_adjmat->[$i][$j]) && 
		(ref($ra_adjmat->[$i][$j]) eq 'ARRAY') && 
		@{$ra_adjmat->[$i][$j]}){ # transitive closure
		$mat[$i][$j] = 1;
		$argc++;
		$argcpercol++;
	    }else{
		$mat[$i][$j] = 0;
	    }
	}
	$tokenc++ if ($argcpercol > 0);
    }
    for my $k (0..$dim){
	for my $i (0..$dim){
	    for my $j (0..$dim){
		$mat[$i][$j] = $mat[$i][$j] || ($mat[$i][$k] && $mat[$k][$j]);
	    }
	}
    }
    return [\@mat, $argc, $tokenc]; # @mat stores the closure, $argc the number of distributed args, $sharedc the number of tokens that expresses these args
}



sub closure_to_stdout($){ # prints to stdout the closure of a dependency graph
    my $ra_mat = shift;
    my $dim = @{$ra_mat}-1;
    for my $i (0..$dim){
	for my $j (0..$dim){
	    print $ra_mat->[$i][$j], " ";
	}
	print "\n";
    }
    print "\n";
}

sub adjmat_to_stdout($$){ # print to stdout an adjacency matrix
    my ($dim, $ra_adjmat) = @_;
    for my $i (0..$dim){
	for my $j (0..$dim){
	    if (defined($ra_adjmat->[$i][$j]) && (ref($ra_adjmat->[$i][$j]) eq 'ARRAY')){
		print "[", join(",", ), @{$ra_adjmat->[$i][$j]}, "] ";
	    }else{
		print "[$i,$j] ";
	    }
	}
	print "\n";
    }
    print "\n";
}

sub synt_adjmat($$){ # returns the labeled syntactic dependency matrix
    my ($dim, $ra_columns) = @_;
    croak if (@{$ra_columns->[8]} != @{$ra_columns->[9]});
    my @adjmat; 
    for my $i (0..$dim){
	for my $j (0..$dim){
	    if ($i == $ra_columns->[8]->[$j]){
		push @{$adjmat[$i][$j]}, $ra_columns->[9]->[$j];
	    }
	}
    }
    return \@adjmat;
}

sub srl_adjmats($$){ # returns the labeled semantic dependency matrices
    my ($dim, $ra_columns) = @_;
    my @adjmat;
    my @nomb_adjmat;
    my @propb_adjmat;
    my @pindexes;
    my $predc = 0; my $propb_predc = 0; my $nomb_predc = 0;
    for my $i (0..$dim){
	if (($ra_columns->[10]->[$i] ne '_') && ($ra_columns->[10]->[$i] ne '-1')){
	    push @pindexes, $i;
	    if ($ra_columns->[3]->[$i] !~ /^VB/){ # NomBank predicate
		$nomb_predc++;
	    }else{ # PropBank predicate
		$propb_predc++;
	    }
	}
    }
    $predc += @pindexes;
    croak if (@{$ra_columns}-11 != @pindexes);
    for my $col (11..@{$ra_columns}-1){
	my $pindex = shift @pindexes;
	croak if (@{$ra_columns->[10]} != @{$ra_columns->[$col]});
	for my $i (0..$dim){
	    # if ((my $srl = $ra_columns->[$col]->[$i]) !~ /^(\-1)|(\_)/){
	    if (($ra_columns->[$col]->[$i] ne '_') && ($ra_columns->[$col]->[$i] ne '-1')){
		push @{$adjmat[$pindex][$i]}, $ra_columns->[$col]->[$i].':'.$ra_columns->[10]->[$pindex];
		if ($ra_columns->[3]->[$pindex] !~ /^VB/){ # NomBank srl
		    push @{$nomb_adjmat[$pindex][$i]}, $ra_columns->[$col]->[$i].':'.$ra_columns->[10]->[$pindex];
		}else{ # PropBank srl
		    push @{$propb_adjmat[$pindex][$i]}, $ra_columns->[$col]->[$i].':'.$ra_columns->[10]->[$pindex];
		}
	    }	    
	}
    }
    croak if (@pindexes);
    return [\@adjmat, \@propb_adjmat, \@nomb_adjmat, $propb_predc, $nomb_predc]; # @adjmat stores the union of @propb_adjmat and @nomb_adjmat
}

END { }

1;
