#!/usr/bin/perl
use warnings;
use strict;
use Carp;

$/ = "";
my $nrec = 0;

my $bolatexd = "\\documentclass[9pt]{minimal}\n\\usepackage{times}\n\\usepackage{latexsym}\n\\usepackage{amssymb,amsmath}\n\\usepackage[color,all]{xy}\n\\begin{document}\n";
my $eolatexd = "\\end{document}";

while (<>){
    # next if (rand() > 0.1);
    $nrec++;
    chomp;
    my $ra_columns = stdin_to_columns($_);
    next if (@{$ra_columns->[0]} != 15);
    my $ra_syntadjmat  = synt_adjmat(@{$ra_columns->[0]}-1, $ra_columns);
    my $ra_srladjmats  = srl_adjmats(@{$ra_columns->[0]}-1, $ra_columns);
    my $hxy = hxy($nrec, $ra_columns, $ra_syntadjmat, $ra_srladjmats);
    print $bolatexd, $hxy, $eolatexd;
    last;
}

sub _srledgel {
    my ($ra_adjmat, $i, $j) = @_;
    if (defined($ra_adjmat->[$i][$j]) && 
	(ref($ra_adjmat->[$i][$j]) eq 'ARRAY') &&
	@{$ra_adjmat->[$i][$j]}){
	my $label = $ra_adjmat->[$i][$j]->[0];
	$label =~ s/\:.*$//;
	return $label;
    }
    return 0;
}


sub _edgel {
    my ($ra_adjmat, $i, $j) = @_;
    if (defined($ra_adjmat->[$i][$j]) && 
	(ref($ra_adjmat->[$i][$j]) eq 'ARRAY') &&
	@{$ra_adjmat->[$i][$j]}){
	my $label = $ra_adjmat->[$i][$j]->[0];
	return lc($label);
    }
    return 0;
}

sub _latex {
    my $s = shift;
    $s =~ s/([\#\$\&\%\_\{\}])/\\$1/g;
    return $s;
}

sub hxy {
    my ($n, $ra_columns, $ra_syntadjmat, $ra_srladjmats) = @_;
    my $xycode = '\begin{displaymath}'."\n".'\xymatrix@C=150pt@R=30pt{'."\n";
    my $arrow = '\ar@/_1pc/';
    my @xymatrix; my $roleset;
    for my $i (1..@{$ra_columns->[5]}-1){
	$xymatrix[$i][0] = "";
	$xymatrix[$i][0] = _latex($ra_columns->[5]->[$i]);
	$xymatrix[$i][1] = "";
	# $roleset = $ra_columns->[10]->[$i] ne '_' ? '/'.$ra_columns->[10]->[$i] : "";;
	$xymatrix[$i][1] = _latex($ra_columns->[5]->[$i]).' _{'.$ra_columns->[3]->[$i].'}';
	$xymatrix[$i][2] = _latex($ra_columns->[5]->[$i]);
	for my $j (1..@{$ra_columns->[5]}-1){
	    if (my $nlabel = _edgel($ra_syntadjmat, $i, $j)){
		my $code = ($i > $j) ? 'u' x ($i-$j) . 'r' : 'd' x ($j-$i) . 'r';
		$code = '[' . $code . ']';
		$xymatrix[$i][0] .= " $arrow".$code.'^<<<<<'."{$nlabel}";
		# print $xymatrix[$i][0], "\n"; exit;
	    }
	    if (my $nlabel = _srledgel($ra_srladjmats->[0], $i, $j)){
		my $code = ($i > $j) ? 'u' x ($i-$j) . 'r' : 'd' x ($j-$i) . 'r';
		$code = '[' . $code . ']';
		$xymatrix[$i][1] .= " $arrow".$code.'^'."{$nlabel}";
		# print $xymatrix[$i][0], "\n"; exit;
	    }
	}
    }
    my $xymatrixcode = "";
    for my $i (1..@{$ra_columns->[5]}-1){
	$xymatrixcode .= $xymatrix[$i][0] . ' & ' . $xymatrix[$i][1] . ' & ' . $xymatrix[$i][2] . "\\"."\\"."\n";
    }
    return $xycode.$xymatrixcode."}\n".'\end{displaymath}'."\n";
}

sub stdin_to_columns { # stores an input CoNLL record in the output array
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

sub synt_adjmat { # returns the labeled syntactic dependency matrix
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

sub srl_adjmats { # returns the labeled semantic dependency matrices
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
