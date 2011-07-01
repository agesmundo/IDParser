#!/usr/bin/perl
use warnings;
use strict;
use DepGraph;
use Carp;

$/ = "";
my $nrec = 0;

while (<>){
    # next if (rand() > 0.1);
    $nrec++;
    chomp;
    my $ra_columns = stdin_to_columns($_);
    
    my $ra_syntadjmat  = synt_adjmat(@{$ra_columns->[0]}-1, $ra_columns);
    my $ra_srladjmats  = srl_adjmats(@{$ra_columns->[0]}-1, $ra_columns);
    
    my $dotcode = dot($nrec, $ra_columns, $ra_syntadjmat, $ra_srladjmats);
    print $dotcode;
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

sub dot {
    my ($n, $ra_columns, $ra_syntadjmat, $ra_srladjmats) = @_;
    my @colors = ("aquamarine","gold","green","red","blue","chocolate","brown","deeppink","orchid");
    my $dotcode = "digraph CoNLL$n {\n";
    $dotcode .= 
	"\tgraph [bgcolor=\"gray71\",rotate=90,ordering=out,ranksep=0.3,nodesep=0.75,size=\"8,10.5!\",margin=0.25];";
    my $nstring = ""; my $estring = ""; my $roleset; my $color;
    for my $i (1..@{$ra_columns->[5]}-1){
	$roleset = $ra_columns->[10]->[$i] ne '_' ? $ra_columns->[10]->[$i] : "";
	$color = "white";
	$color = $colors[$i % @colors] if ($roleset);
	$nstring .= 
	    "\tn$i [label=\"$i:$ra_columns->[5]->[$i]\\n$ra_columns->[3]->[$i]\\n$roleset\",style=filled,color=\"$color\"]\n";
	
    }
    for my $j (1..@{$ra_columns->[0]}-1){
	for my $i (1..@{$ra_columns->[0]}-1){
	    if ($i != 0 && (my $elabel = _edgel($ra_syntadjmat,$i,$j))){
		$estring .= "\tn$i -> n$j [label=\"$elabel\",constraint=true,style=bold];\n";
	    }
	    if ($i != 0 && (my $elabel = _srledgel($ra_srladjmats->[0],$i,$j))){
		$color = "white";
		$color = $colors[$i % @colors];
		$estring .= 
		    "\tn$i -> n$j [label=\"$elabel\",constraint=false,style=bold,fontcolor=\"$color\",color=\"$color\"];\n";
	    }
	}
    }
    my $sent = join(" ", @{$ra_columns->[5]}[1..@{$ra_columns->[5]}-1]);
    return $dotcode . "\n" . $nstring . "\n" . $estring . "\n\tlabel=\"$sent\";" . "\n}\n";
}
