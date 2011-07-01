#!/usr/bin/perl

package CoNLL;

use strict;
use warnings;
use Carp;

BEGIN {
    use Exporter   ();
    our ($VERSION, @ISA, @EXPORT, @EXPORT_OK, @EXPORT_FAIL, %EXPORT_TAGS);
    @ISA         = qw(Exporter);
    @EXPORT      = qw(&conllrec_to_array &conllarray_to_rowstring &conllarray_to_columnstring &conllarray_to_srlstring
		      &conllarray_to_ivanarray &conllarray_to_ivaneaclarray
		      &syntadjmat &srladjmats &adjmat_to_string &srlstruct &connectedsrladjmat
		      &closure &closure_to_string &closurerec_to_array
		      &ivanprojmat &marcusprojmat &projmat_to_string &isivanproj &ismarcusproj
		      &conll07array_to_conllarray &conll07array_to_conllarraySUrm &conllarray_to_conll07array
		      &conllarray_to_conllarraySUrm &dot &projectivize &projsrladjmat_to_conll &setprojconll
		      &itjhinvproj_to_array &itjhrec_to_srladjmat &itjhadjmat_to_conlladjmat
		      &_srlprojstruct &_ivannonprojdegree &srl_adjmats &_sortedancestorspan &rara_to_string
		      &projectivise_srladjmat
		      &srl_confusionm &confm_to_string &confm_to_f1_string
		      &is_cprojective &is_cplanar &cplanaritydegree &csemdgclosure &csemdg
		      &csyntsemdg &csyntsemdg &cplanarityiodegree
		      &depsrl_adjmats &intersection &include &union &difference &symmetricdifference &comparison &nonplanarminsubg
		      &sx_adjmat &ss_adjmat &pr
		      );
}

our %conllf = ('ID' => 0, 'FORM' => 1, 'LEMMA' => 2, 'GPOS' => 3, 
	       'PPOS' => 4, 'SPLIT_FORM' => 5, 'SPLIT_LEMMA' => 6, 'PPOSS' => 7, 
	       'HEAD' => 8, 'DEPREL' => 9, 'PRED' => 10, 'ARG' => 11);
our %ivanf = ('ID' => 0, 'SFORM' => 1, 'SLEMMA' => 2, 'CPOS' => 3, 
	      'POS' => 4, 'FEATS' => 5, 'BANK' => 6, 'HEAD' => 7, 
	      'DEPREL' => 8, 'SENSE' => 9, 'ARG-SRL' => 10);

our %conll07f = ('ID' => 0, 'FORM' => 1, 'LEMMA' => 2, 'CPOSTAG' => 3, 
		 'POSTAG' => 4, 'FEATS' => 5, 'HEAD' => 6, 'DEPREL' => 7, 
		 'PHEAD' => 8, 'PDEPREL' => 9);


sub _srledgel {
    my ($ra_adjmat, $i, $j) = @_;
    if ($ra_adjmat->[$i][$j]){
	my $label = $ra_adjmat->[$i][$j]->[0];
	$label =~ s/[\:\*].*$//;
	return $label;
    }
    return 0;
}


sub _edgel {
    my ($ra_adjmat, $i, $j) = @_;
    if ($ra_adjmat->[$i][$j]){
	my $label = $ra_adjmat->[$i][$j]->[0];
	return lc($label);
    }
    return 0;
}

sub dot {
    my ($n, $ra_columns, $ra_syntadjmat, $ra_srladjmat) = @_;
    my @colors = ("aquamarine","gold","green","red","blue","chocolate","brown","deeppink","orchid");
    my $dotcode = "digraph CoNLL$n {\n";
    $dotcode .= 
	"\tgraph [bgcolor=\"gray71\",rotate=90,ordering=out,ranksep=0.3,nodesep=0.75,size=\"8,10.5!\",margin=0.25];";
    my $nstring = ""; my $estring = ""; my $roleset; my $color;
    for my $i (1..@{$ra_columns->[$conllf{'SPLIT_FORM'}]}-1){
	$roleset = $ra_columns->[$conllf{'PRED'}]->[$i] ne '_' ? $ra_columns->[$conllf{'PRED'}]->[$i] : "";
	$color = "white";
	$color = $colors[$i % $#colors] if ($roleset);
	$nstring .= "\tn$i ";
	$nstring .= 
	    "[label=\"$i:$ra_columns->[$conllf{'SPLIT_FORM'}]->[$i]" .
	    "\\n$ra_columns->[$conllf{'GPOS'}]->[$i]\\n$roleset\",style=filled,color=\"$color\"]\n";
	
    }
    for my $j (1..@{$ra_columns->[0]}-1){
	for my $i (1..@{$ra_columns->[0]}-1){
	    if ($i != 0 && (my $elabel = _edgel($ra_syntadjmat,$i,$j))){
		$estring .= "\tn$i -> n$j [label=\"$elabel\",constraint=true,style=bold];\n";
	    }
	    if ($i != 0 && (my $elabel = _srledgel($ra_srladjmat,$i,$j))){
		$color = "white";
		$color = $colors[$i % $#colors];
		$estring .= "\tn$i -> n$j ";
		$estring .= 
		    "[label=\"$elabel\",constraint=false,style=bold,fontcolor=\"$color\",color=\"$color\"];\n";
	    }
	}
    }
    my $sent = join(" ", @{$ra_columns->[$conllf{'SPLIT_FORM'}]}[1..@{$ra_columns->[$conllf{'SPLIT_FORM'}]}-1]);
    return $dotcode . "\n" . $nstring . "\n" . $estring . "\n\tlabel=\"$sent\";" . "\n}\n";
}

sub conllarray_to_conllarraySUrm { # rm SU predicates and SU arguments
    my $ra_conllarray = shift;
    my @conll;
    foreach my $ra (@{$ra_conllarray}[$conllf{'ID'}..$conllf{'DEPREL'}]){
	push @conll, $ra;
    }
    my $k = 1;
    my @a;
    foreach my $i (0..@{$ra_conllarray->[$conllf{'PRED'}]}-1){
	$conll[$conllf{'PRED'}]->[$i] = $ra_conllarray->[$conllf{'PRED'}]->[$i];
	next if ($conll[$conllf{'PRED'}]->[$i] eq '_' or $conll[$conllf{'PRED'}]->[$i] eq '-1');
	my ($lemma, $sense) = split /\./, $conll[$conllf{'PRED'}]->[$i];
	if (defined($sense) && ($sense ne 'SU')){
	    my @a = map { $_ } @{$ra_conllarray->[$conllf{'PRED'}+$k]};
	    push @conll, \@a; # deep copy
	    
	}else{
	    $conll[$conllf{'PRED'}]->[$i] = '_';
	}
	$k++;
    }
    foreach my $i ($conllf{'ARG'}..@conll-1){# @{$ra_conllarray}-1){
	# next if (!defined($conll[$i]));
	foreach my $a (0..@{$conll[$i]}-1){
	    $conll[$i]->[$a] = '_' if ($conll[$i]->[$a] eq 'SU');
	}
    }
    return \@conll;
}

sub conll07array_to_conllarraySUrm { # rm SU predicates and SU arguments
    my ($ra_conll07array, $ra_conllarray) = @_;
    my @conll;
    foreach my $ra (@{$ra_conllarray}[$conllf{'ID'}..$conllf{'DEPREL'}]){
	my @cp = map {$_} @{$ra};
	push @conll, \@cp;
    }
    @{$conll[$conllf{'HEAD'}]} = map {$_} @{$ra_conll07array->[$conll07f{'HEAD'}]}; # deep copy
    @{$conll[$conllf{'DEPREL'}]} = map {$_} @{$ra_conll07array->[$conll07f{'DEPREL'}]}; # deep copy
    my $k = 0;
    foreach my $i (0..@{$ra_conllarray->[$conllf{'PRED'}]}-1){
	$conll[$conllf{'PRED'}]->[$i] = $ra_conllarray->[$conllf{'PRED'}]->[$i];
	next if ($conll[$conllf{'PRED'}]->[$i] eq '_');
	my ($lemma, $sense) = split /\./, $conll[$conllf{'PRED'}]->[$i];
	if (defined($sense) && ($sense ne 'SU')){
	    my @a = map { $_ } $ra_conllarray->[$conllf{'PRED'}+$k];
	    push @conll, \@a;
	}else{
	    $conll[$conllf{'PRED'}]->[$i] = '_';
	}
	$k++;
    }
    foreach my $i ($conllf{'ARG'}..@conll-1){
	foreach my $a (0..@{$conll[$i]}-1){
	    $conll[$i]->[$a] = '_' if ($conll[$i]->[$a] eq 'SU');
	}
    }
    return \@conll;
}

sub conll07array_to_conllarray {
    my ($ra_conll07array, $ra_conllarray) = @_;
    my @conll;
    foreach my $ra (@{$ra_conllarray}){
	my @cp = map {$_;} @{$ra};
	push @conll, \@cp;
    }
    @{$conll[$conllf{'HEAD'}]} = map {$_} @{$ra_conll07array->[$conll07f{'HEAD'}]}; # deep copy
    @{$conll[$conllf{'DEPREL'}]} = map {$_} @{$ra_conll07array->[$conll07f{'DEPREL'}]}; # deep copy
    return \@conll;
}

sub conllarray_to_conll07array {
    my $ra_array = shift;
    my @conll07;
    $conll07[$conll07f{'ID'}] = $ra_array->[$conllf{'ID'}];
    $conll07[$conll07f{'FORM'}] = $ra_array->[$conllf{'SPLIT_FORM'}];
    $conll07[$conll07f{'LEMMA'}] = $ra_array->[$conllf{'SPLIT_LEMMA'}];
    $conll07[$conll07f{'CPOSTAG'}] = $ra_array->[$conllf{'PPOSS'}];
    $conll07[$conll07f{'POSTAG'}] = $ra_array->[$conllf{'PPOSS'}];
    @{$conll07[$conll07f{'FEATS'}]} = map { '_' } @{$ra_array->[$conllf{'ID'}]};
    $conll07[$conll07f{'HEAD'}] = $ra_array->[$conllf{'HEAD'}];
    $conll07[$conll07f{'DEPREL'}] = $ra_array->[$conllf{'DEPREL'}];
    @{$conll07[$conll07f{'PHEAD'}]} = map { '_' } @{$ra_array->[$conllf{'ID'}]};
    @{$conll07[$conll07f{'PDEPREL'}]} = map { '_' } @{$ra_array->[$conllf{'ID'}]};
    return \@conll07;
}


sub conllarray_to_splitpost_ivanarray {
    my $ra_array = shift;
    my $rh_pb = shift; my $rh_nb = shift;
    my @aivan;
    $aivan[$ivanf{'ID'}] = $ra_array->[$conllf{'ID'}];
    $aivan[$ivanf{'SFORM'}] = $ra_array->[$conllf{'SPLIT_FORM'}];
    $aivan[$ivanf{'SLEMMA'}] = $ra_array->[$conllf{'SPLIT_LEMMA'}];
    $aivan[$ivanf{'CPOS'}] = $ra_array->[$conllf{'PPOSS'}];
    $aivan[$ivanf{'POS'}] = $ra_array->[$conllf{'PPOSS'}];
    $aivan[$ivanf{'HEAD'}] = $ra_array->[$conllf{'HEAD'}];
    $aivan[$ivanf{'DEPREL'}] = $ra_array->[$conllf{'DEPREL'}];
    @{$aivan[$ivanf{'SENSE'}]} = map { $_ } @{$ra_array->[$conllf{'PRED'}]}; # deep copy
    foreach my $i (0..@{$aivan[$ivanf{'ID'}]}-1){
	$aivan[$ivanf{'BANK'}]->[$i] = '-1';
	if (exists($rh_pb->{$aivan[$ivanf{'SLEMMA'}]->[$i]}) || exists($rh_nb->{$aivan[$ivanf{'SLEMMA'}]->[$i]})){
	    if (($ra_array->[$conllf{'PPOSS'}]->[$i] =~ /^NN/) &&
		($ra_array->[$conllf{'GPOS'}]->[$i] =~ /^NN/)){
		$aivan[$ivanf{'BANK'}]->[$i] = 1;
	    }elsif (($ra_array->[$conllf{'PPOSS'}]->[$i] =~ /^VB/) &&
		    ($ra_array->[$conllf{'GPOS'}]->[$i] =~ /^VB/)){
		$aivan[$ivanf{'BANK'}]->[$i] = 0;
	    }elsif ($ra_array->[$conllf{'PPOSS'}]->[$i] eq $ra_array->[$conllf{'GPOS'}]->[$i]){
		$aivan[$ivanf{'BANK'}]->[$i] = 0;
	    }else{
		$aivan[$ivanf{'SENSE'}]->[$i] = '_';
	    }
	}else{
	    $aivan[$ivanf{'SENSE'}]->[$i] = '_';
	}
	$aivan[$ivanf{'FEATS'}]->[$i] = '_';
	if ($aivan[$ivanf{'DEPREL'}]->[$i] eq 'HMOD'){
	    if (($i > 0) && ($aivan[$ivanf{'DEPREL'}]->[$i-1] ne 'HYPH')){
		$aivan[$ivanf{'FEATS'}]->[$i] = 'BOHYPH';
	    }else {
		$aivan[$ivanf{'FEATS'}]->[$i] = 'COHYPH';
	    }
	}
	if (($aivan[$ivanf{'DEPREL'}]->[$i-1] eq 'HMOD') && ($aivan[$ivanf{'DEPREL'}]->[$i] eq 'HYPH')){
	    $aivan[$ivanf{'FEATS'}]->[$i] = 'COHYPH';
	}
	if (($aivan[$ivanf{'DEPREL'}]->[$i-1] eq 'HYPH') && ($aivan[$ivanf{'DEPREL'}]->[$i] ne 'HMOD')){
	    $aivan[$ivanf{'FEATS'}]->[$i] = 'EOHYPH';
	} 
	$aivan[$ivanf{'ARG-SRL'}]->[$i] = '';
    }
    $aivan[$ivanf{'FEATS'}]->[0] = '-1';
    $aivan[$ivanf{'BANK'}]->[0] = '-1';
    my $k = 1;
    foreach my $i (1..@{$ra_array->[$conllf{'PRED'}]}-1){
	next if ($ra_array->[$conllf{'PRED'}]->[$i] eq '_');
	if ($aivan[$ivanf{'SENSE'}]->[$i] eq '_'){
	    $k++;next;
	}
	foreach my $j (1..@{$ra_array->[$conllf{'PRED'}+$k]}-1){
	    next if ($i == $j); # loopy structures are not taken into consideration
	    next if ($ra_array->[$conllf{'PRED'}+$k]->[$j] eq '_');
	    $aivan[$ivanf{'ARG-SRL'}]->[$i] .= "\t" . $j . "\t" . $ra_array->[$conllf{'PRED'}+$k]->[$j];
	    if ($ra_array->[$conllf{'PRED'}+$k]->[$j] =~ 
		/(ADV)|(CAU)|(DIR)|(DIS)|(EXT)|(LOC)|(MNR)|(PNC)|(TMP)/){
		$aivan[$ivanf{'CPOS'}] = $ra_array->[$conllf{'PPOSS'}] . '-' . $ra_array->[$conllf{'PRED'}+$k]->[$j];
		$aivan[$ivanf{'POS'}] = $ra_array->[$conllf{'PPOSS'}] . '-' . $ra_array->[$conllf{'PRED'}+$k]->[$j];

	    }
	}
	$k++;
    }
    return \@aivan;
}


sub conllarray_to_ivanarray {
    my $ra_array = shift;
    my $rh_pb = shift; my $rh_nb = shift;
    my @aivan;
    $aivan[$ivanf{'ID'}] = $ra_array->[$conllf{'ID'}];
    $aivan[$ivanf{'SFORM'}] = $ra_array->[$conllf{'SPLIT_FORM'}];
    $aivan[$ivanf{'SLEMMA'}] = $ra_array->[$conllf{'SPLIT_LEMMA'}];
    $aivan[$ivanf{'CPOS'}] = $ra_array->[$conllf{'PPOSS'}];
    $aivan[$ivanf{'POS'}] = $ra_array->[$conllf{'PPOSS'}];
    $aivan[$ivanf{'HEAD'}] = $ra_array->[$conllf{'HEAD'}];
    $aivan[$ivanf{'DEPREL'}] = $ra_array->[$conllf{'DEPREL'}];
    @{$aivan[$ivanf{'SENSE'}]} = map { $_ } @{$ra_array->[$conllf{'PRED'}]}; # deep copy
    foreach my $i (0..@{$aivan[$ivanf{'ID'}]}-1){
	$aivan[$ivanf{'BANK'}]->[$i] = '-1';
	if (exists($rh_pb->{$aivan[$ivanf{'SLEMMA'}]->[$i]}) || exists($rh_nb->{$aivan[$ivanf{'SLEMMA'}]->[$i]})){
	    if (($ra_array->[$conllf{'PPOSS'}]->[$i] =~ /^NN/) &&
		($ra_array->[$conllf{'GPOS'}]->[$i] =~ /^NN/)){
		$aivan[$ivanf{'BANK'}]->[$i] = 1;
	    }elsif (($ra_array->[$conllf{'PPOSS'}]->[$i] =~ /^VB/) &&
		    ($ra_array->[$conllf{'GPOS'}]->[$i] =~ /^VB/)){
		$aivan[$ivanf{'BANK'}]->[$i] = 0;
	    }elsif ($ra_array->[$conllf{'PPOSS'}]->[$i] eq $ra_array->[$conllf{'GPOS'}]->[$i]){
		$aivan[$ivanf{'BANK'}]->[$i] = 0;
	    }else{
		$aivan[$ivanf{'SENSE'}]->[$i] = '_';
	    }
	}else{
	    $aivan[$ivanf{'SENSE'}]->[$i] = '_';
	}
	$aivan[$ivanf{'FEATS'}]->[$i] = '_' if (!defined($aivan[$ivanf{'FEATS'}]->[$i]));
	if (($i > 0) and ($i < (@{$aivan[$ivanf{'ID'}]}-1))){
	    if (($aivan[$ivanf{'DEPREL'}]->[$i] eq 'HYPH') or 
		($aivan[$ivanf{'GPOS'}]->[$i] eq 'HYPH')){
		$aivan[$ivanf{'FEATS'}]->[$i] = 'COHYPH';
		if ($aivan[$ivanf{'FEATS'}]->[$i-1] eq '_'){
		    $aivan[$ivanf{'FEATS'}]->[$i-1] = 'BOHYPH';
		}else{
		    $aivan[$ivanf{'FEATS'}]->[$i-1] = 'COHYPH';
		}
		$aivan[$ivanf{'FEATS'}]->[$i+1] = 'EOHYPH';
	    }
	}
	$aivan[$ivanf{'ARG-SRL'}]->[$i] = '';
    }
    $aivan[$ivanf{'FEATS'}]->[0] = '-1';
    $aivan[$ivanf{'BANK'}]->[0] = '-1';
    my $k = 1;
    foreach my $i (1..@{$ra_array->[$conllf{'PRED'}]}-1){
	next if ($ra_array->[$conllf{'PRED'}]->[$i] eq '_');
	if ($aivan[$ivanf{'SENSE'}]->[$i] eq '_'){
	    $k++;next;
	}
	foreach my $j (1..@{$ra_array->[$conllf{'PRED'}+$k]}-1){
	    next if ($i == $j); # loopy structures are not taken into consideration
	    next if ($ra_array->[$conllf{'PRED'}+$k]->[$j] eq '_');
	    $aivan[$ivanf{'ARG-SRL'}]->[$i] .= "\t" . $j . "\t" . $ra_array->[$conllf{'PRED'}+$k]->[$j]; 
	}
	$k++;
    }
    return \@aivan;
}

sub conllarray_to_ivaneaclarray {
    my $ra_array = shift;
    my $rh_pb = shift; my $rh_nb = shift;
    my @aivan;
    $aivan[$ivanf{'ID'}] = $ra_array->[$conllf{'ID'}];
    $aivan[$ivanf{'SFORM'}] = $ra_array->[$conllf{'SPLIT_FORM'}];
    $aivan[$ivanf{'SLEMMA'}] = $ra_array->[$conllf{'SPLIT_LEMMA'}];
    $aivan[$ivanf{'CPOS'}] = $ra_array->[$conllf{'PPOSS'}];
    $aivan[$ivanf{'POS'}] = $ra_array->[$conllf{'PPOSS'}];
    $aivan[$ivanf{'HEAD'}] = $ra_array->[$conllf{'HEAD'}];
    $aivan[$ivanf{'DEPREL'}] = $ra_array->[$conllf{'DEPREL'}];
    @{$aivan[$ivanf{'SENSE'}]} = map { $_ } @{$ra_array->[$conllf{'PRED'}]}; # deep copy
    foreach my $i (0..@{$aivan[$ivanf{'ID'}]}-1){
	$aivan[$ivanf{'BANK'}]->[$i] = '-1';
	if (exists($rh_pb->{$aivan[$ivanf{'SLEMMA'}]->[$i]}) || exists($rh_nb->{$aivan[$ivanf{'SLEMMA'}]->[$i]})){
	    if (($ra_array->[$conllf{'PPOSS'}]->[$i] =~ /^NN/) &&
		($ra_array->[$conllf{'GPOS'}]->[$i] =~ /^NN/)){
		$aivan[$ivanf{'BANK'}]->[$i] = 1;
	    }elsif (($ra_array->[$conllf{'PPOSS'}]->[$i] =~ /^VB/) &&
		    ($ra_array->[$conllf{'GPOS'}]->[$i] =~ /^VB/)){
		$aivan[$ivanf{'BANK'}]->[$i] = 0;
	    }elsif ($ra_array->[$conllf{'PPOSS'}]->[$i] eq $ra_array->[$conllf{'GPOS'}]->[$i]){
		$aivan[$ivanf{'BANK'}]->[$i] = 0;
	    }else{
		$aivan[$ivanf{'SENSE'}]->[$i] = '_';
	    }
	}else{
	    $aivan[$ivanf{'SENSE'}]->[$i] = '_';
	}
	$aivan[$ivanf{'FEATS'}]->[$i] = '_' if (!defined($aivan[$ivanf{'FEATS'}]->[$i]));
	if (($i > 0) and ($i < (@{$aivan[$ivanf{'ID'}]}-1))){
	    if ($aivan[$ivanf{'DEPREL'}]->[$i] eq 'HYPH'){
		$aivan[$ivanf{'FEATS'}]->[$i] = 'COHYPH';
		if ($aivan[$ivanf{'FEATS'}]->[$i-1] eq '_'){
		    $aivan[$ivanf{'FEATS'}]->[$i-1] = 'BOHYPH';
		}else{
		    $aivan[$ivanf{'FEATS'}]->[$i-1] = 'COHYPH';
		}
		$aivan[$ivanf{'FEATS'}]->[$i+1] = 'EOHYPH';
	    }
	}
	$aivan[$ivanf{'ARG-SRL'}]->[$i] = '';
    }
    $aivan[$ivanf{'FEATS'}]->[0] = '-1';
    $aivan[$ivanf{'BANK'}]->[0] = '-1';
    my $k = 1;
    foreach my $i (1..@{$ra_array->[$conllf{'PRED'}]}-1){
	next if ($ra_array->[$conllf{'PRED'}]->[$i] eq '_');
	if ($aivan[$ivanf{'SENSE'}]->[$i] eq '_'){
	    $k++;next;
	}
	foreach my $j (1..@{$ra_array->[$conllf{'PRED'}+$k]}-1){
	    # next if ($i == $j); # loopy structures are not taken into consideration
	    next if ($ra_array->[$conllf{'PRED'}+$k]->[$j] eq '_');
	    if ($i == $j){
		$aivan[$ivanf{'SENSE'}]->[$i] .= '~' . $ra_array->[$conllf{'PRED'}+$k]->[$j];
	    }else{
		$aivan[$ivanf{'ARG-SRL'}]->[$i] .= "\t" . $j . "\t" . $ra_array->[$conllf{'PRED'}+$k]->[$j]; 
	    }
	}
	$k++;
    }
    return \@aivan;
}


sub projmat_to_string($){
    my $ra_projmat = shift;
    my $s = '';
    for my $i (0..@{$ra_projmat}-1){
	for my $j (0..@{$ra_projmat}-1){
	    $s .= $ra_projmat->[$i][$j] . " ";
	}
	$s .= "\n";
    }
    return $s . "\n";
}

sub _sortedancestorspan {
    my ($p, $arg, $ra_closure) = @_;
    my @ancestors;
    my ($i, $j) = ($p < $arg) ? ($p, $arg) : ($arg, $p); 
    foreach my $g ($i+1..$j-1){
	push @ancestors, [$g, $ra_closure->[$g][$p]] if ($ra_closure->[$g][$p]);
    }
    # my @sorted = map {$_->[0]} sort {$a->[1] <=> $b->[1]} @ancestors;
    # return @sorted;
    return [map {$_->[0]} sort {$a->[1] <=> $b->[1]} @ancestors];
}

sub _leastdeepgov {
    my ($p, $a, $ra_syntadjmat, $ra_closure) = @_;
    my ($predlabel, $arglabel, $depth, $sdistance, $ldistance, $gov);
    foreach my $i (0..@{$ra_syntadjmat->[0]}-1){
	if ($ra_syntadjmat->[0]->[$i][$p]){
	    $predlabel = $ra_syntadjmat->[0]->[$i][$p]->[0];
	}
	if ($ra_syntadjmat->[0]->[$i][$a]){
	    $arglabel = $ra_syntadjmat->[0]->[$i][$a]->[0];
	}
    }
    $depth = $ra_closure->[0]->[$p];
    $sdistance = $ra_closure->[$p][$a];
    if ($sdistance == 0){
	$sdistance = -1 if ($p != $a);
    }
    $ldistance = abs($p - $a);
    my ($i, $j) = ($p < $a) ? ($p, $a) : ($a, $p); 
    $gov = 0;
    foreach my $g ($i+1..$j-1){
	$gov = $g if ($ra_closure->[$g][$p] > $gov);
	# push @ancestors, [$g, $ra_closure->[$g][$p]] if ($ra_closure->[$g][$p]);
    }
    # sort {$a->[1] <=> $b->[0]} @ancestors;
    return ($predlabel, $arglabel, $depth, $sdistance, $gov, $ldistance);
}

sub _srlprojstruct { # returns an unsorted array storing PropBank and NomBank data for projectivisation
    my ($ra_syntadjmat, $ra_closure, $ra_srladjmats) = @_;
    my $root = 0;
    foreach my $i (0..@{$ra_closure->[0]}-1){
	if ($ra_closure->[0][$i] == 1){
	    $root = $i; last;
	}
    }
    foreach my $nb (@{$ra_srladjmats->[8]}){ # range over NomBank srls
	push @{$nb}, _leastdeepgov($nb->[0], $nb->[1], $ra_syntadjmat, $ra_closure);
	push @{$nb}, _ivannonprojdegree($ra_srladjmats, $nb->[0], $nb->[1]);
	push @{$nb}, 0;
	push @{$nb}, $root;
	# my $ra_ancestors = _sortedancestorspan($nb->[0], $nb->[1], $ra_closure);
	# print STDERR "$nb->[0] $nb->[1] > ", join(" * ", @{$ra_ancestors}), "\n";
    }
    foreach my $pb (@{$ra_srladjmats->[7]}){ # range over PropBank srls
	push @{$pb}, _leastdeepgov($pb->[0], $pb->[1], $ra_syntadjmat, $ra_closure);
	push @{$pb}, _ivannonprojdegree($ra_srladjmats, $pb->[0], $pb->[1]);
	push @{$pb}, 1;
	push @{$pb}, $root;
	# my $ra_ancestors = _sortedancestorspan($pb->[0], $pb->[1], $ra_closure);
	# print STDERR "$pb->[0] $pb->[1] > ", join(" : ", @{$ra_ancestors}), "\n";
    }

    # 0: pred index, 
    # 1: arg index,
    # 2: srl
    # 3: syntactic label licensing the predicate
    # 4: syntactic label licensing the argument
    # 5: depth of the predicate
    # 6: syntactic distance from the predicate to the argument (-1 if the predicate is not an ancestor of the argument)
    # 7: index of the least deep gov of the predicate
    # 8: linear distance
    # 9: non projectivity degree
    # 10: 0(NomBank), 1(PropBank)
    # 11: root index
}


sub setprojconll {
    my ($ra_conll, $ra_srlconll) = @_;
    my @projconll;
    foreach my $ra_field (@{$ra_conll}[0..$conllf{'PRED'}-1]){
	my @f = map { $_ } @{$ra_field};
	push @projconll, \@f; # deep copy
    }
    foreach my $ra_srl (@{$ra_srlconll}){
	my @f = map { $_ } @{$ra_srl};
	push @projconll, \@f; # deep copy
    } 
    return \@projconll;
}

sub projsrladjmat_to_conll { # format projectivised srl adjmat
    my ($ra_adjmat, $ra_conll) = @_;
    my (@srlcol, @connv);
    for my $i (0..@{$ra_adjmat}-1){
	my $p = 0;
	my @args;
	for my $j (0..@{$ra_adjmat}-1){
	    $args[$j] = '_';
	    $p = 1 if ($ra_adjmat->[$i]->[$j]);
	    if ($ra_adjmat->[$i]->[$j]){
		# $args[$j] = join(",", @{$ra_adjmat->[$i]->[$j]});
		# to relabel
		my @labels;
		foreach my $l (@{$ra_adjmat->[$i]->[$j]}){
		    push @labels, ($l =~ /(^[^:*]+)[:*]/);
		}
		$connv[$i]++; $connv[$j]++;
		$args[$j] = join(",", @labels);
	    }
	}
	if ($p){
	    # split label
	    if (($ra_conll->[$conllf{'PRED'}]->[$i] ne '_') && ($ra_conll->[$conllf{'PRED'}]->[$i] ne '-1')){
		$srlcol[0]->[$i] = $ra_conll->[$conllf{'PRED'}]->[$i];		
	    }else{
		$srlcol[0]->[$i] = $ra_conll->[$conllf{'LEMMA'}]->[$i] . '.pp';
	    }
	    push @srlcol, \@args;
	}elsif (($ra_conll->[$conllf{'PRED'}]->[$i] ne '_') && ($ra_conll->[$conllf{'PRED'}]->[$i] ne '-1')){
	    $srlcol[0]->[$i] = $ra_conll->[$conllf{'PRED'}]->[$i];
	    push @srlcol, \@args;
	}else{
	    $srlcol[0]->[$i] = '_';
	}
    }
    return \@srlcol;
}

#sub projectivise_nbsrladjmat {
#}

#sub projectivise_pbsrladjmat {
#}


sub _iscrossingedge {
    my ($ra_adjmat, $pred, $arg) = @_;
    return 0 if ($pred == $arg);
    my ($l, $u) = ($pred, $arg); 
    ($l, $u) = ($arg, $pred) if ($pred > $arg); 
    for my $k ($l+1..$u-1){
	for my $m (0..$l-1){
	    if ((ref($ra_adjmat->[$m][$k]) eq 'ARRAY') && @{$ra_adjmat->[$m][$k]}){
		return 1;
	    }
	    if ((ref($ra_adjmat->[$k][$m]) eq 'ARRAY') && @{$ra_adjmat->[$k][$m]}){
		return 1;
	    }
	}
	for my $m ($u+1..@{$ra_adjmat->[$conllf{'ID'}]}-1){
	    if ((ref($ra_adjmat->[$m][$k]) eq 'ARRAY') && @{$ra_adjmat->[$m][$k]}){
		return 1;
	    }
	    if ((ref($ra_adjmat->[$k][$m]) eq 'ARRAY') && @{$ra_adjmat->[$k][$m]}){
		return 1;
	    }
	}
    }
    return 0;
}

sub _liftsrl {
    my ($ra_adjmat, $gov, $dep, $srl) = @_;
    return 0 if (($gov != $dep) && _iscrossingedge($ra_adjmat, $gov, $dep));
    if (ref($ra_adjmat->[$gov][$dep]) eq 'ARRAY'){
	push @{$ra_adjmat->[$gov][$dep]}, $srl;
    }else{
	$ra_adjmat->[$gov][$dep] = [$srl];
    }
    return 1;
}

sub projectivise_srladjmat {
    # 0: pred index, 
    # 1: arg index,
    # 2: srl
    # 3: syntactic label licensing the predicate
    # 4: syntactic label licensing the argument
    # 5: depth of the predicate
    # 6: syntactic distance from the predicate to the argument (-1 if the predicate is not an ancestor of the argument)
    # 7: index of the least deep gov of the predicate
    # 8: linear distance
    # 9: non projectivity degree
    # 10: 0(NomBank), 1(PropBank)
    # 11: root index
    my ($ra_srladjmats, $debug) = @_;
    my @sorted0 = sort { $b->[10] <=> $a->[10] || 
			     $a->[6] <=> $b->[6] || 
			     $b->[9] <=> $a->[9] || 
			     $b->[5] <=> $a->[5]} (@{$ra_srladjmats->[7]}, @{$ra_srladjmats->[8]});
    my @sorted1 = sort { $b->[9] <=> $a->[9] || 
			     $a->[10] <=> $b->[10] || 
			     $b->[6] <=> $a->[6] || 
			     $b->[5] <=> $a->[5]} (@{$ra_srladjmats->[7]}, @{$ra_srladjmats->[8]});
    # nonproj stats
    my ($nbnproj, $pbnproj);
    $nbnproj = 0;
    $pbnproj = 0;
    map { $nbnproj++ if ($_->[9]); } @{$ra_srladjmats->[8]}; # NomBank srl
    map { $pbnproj++ if ($_->[9]); } @{$ra_srladjmats->[7]}; # PropBank srl
    
    my $ra_srladjmat0 = cp_srladjmat($ra_srladjmats->[0]);
    my $ra_srladjmat1 = cp_srladjmat($ra_srladjmats->[0]);
    my $ra_freq0 = _projectivise($ra_srladjmat0, \@sorted0, $debug);
    # $ra_freq-> ($nlifted_nom, $nrm_nom, $nlifted_prop, $nrm_prop)
    print STDERR "SRLADJMAT deepest first:\n\n", adjmat_to_string($ra_srladjmat0) if ($debug);
    print STDERR join(" ", @{$ra_freq0}), "\n" if ($debug);
    my $ra_freq1 =_projectivise($ra_srladjmat1, \@sorted1, $debug);
    print STDERR "SRLADJMAT most crossing first:\n",adjmat_to_string($ra_srladjmat1) if ($debug);
    print STDERR join(" ", @{$ra_freq1}), "\n\n" if ($debug);
    my $minrm = 0; my $minlifted = 0;
    return [$ra_freq0, $ra_freq1, $nbnproj, $pbnproj];
    if (($ra_freq0->[0] + $ra_freq0->[1] + $ra_freq0->[3] + $ra_freq0->[3]) > 
	($ra_freq1->[0] + $ra_freq1->[1] + $ra_freq1->[2] + $ra_freq1->[3])){
	return [$ra_freq0, $ra_freq1, $ra_freq1];
    }else{
	return [$ra_freq0, $ra_freq1, $ra_freq0];
    }
    # minimize the sum of lifted and removed srls
    # or minimize the number of removed srls
    # or minimize the number of lifted srls
    # or minimize the number of removed PropBank srls
    # or minimize the number of lifted PropBank srls
}

sub cp_srladjmat { # deep copy
    my ($ra_srladjmat) = shift;
    my @srladjmat;
    foreach my $i (0..@{$ra_srladjmat}-1){
	foreach my $j (0..@{$ra_srladjmat->[$i]}-1){
	    if (ref($ra_srladjmat->[$i][$j]) eq 'ARRAY'){
		$srladjmat[$i][$j] = [map { $_ } @{$ra_srladjmat->[$i][$j]}];
	    }else{
		$srladjmat[$i][$j] = $ra_srladjmat->[$i][$j];
	    }
	}
    }
    return \@srladjmat;
}


sub _projectivise {
    # 0: pred index, 
    # 1: arg index,
    # 2: srl
    # 3: syntactic label licensing the predicate
    # 4: syntactic label licensing the argument
    # 5: depth of the predicate
    # 6: syntactic distance from the predicate to the argument (-1 if the predicate is not an ancestor of the argument)
    # 7: index of the least deep gov of the predicate
    # 8: linear distance
    # 9: non projectivity degree
    # 10: 0(NomBank), 1(PropBank)
    # 11: root index
    my ($ra_srladjmat, $ra_sorted, $debug) = @_;
    my (@ppsrls, @rmsrls);
    my $nlifted_nom = 0; my $nlifted_prop = 0;
    my $nrm_nom = 0; my $nrm_prop = 0;
    foreach my $srl (@{$ra_sorted}){ 
	print STDERR join("\t", @{$srl}), "\n" if ($debug);
	if ($srl->[9] && _iscrossingedge($ra_srladjmat, $srl->[0], $srl->[1])){ # non-projective srl
	    # dynamically reset srladjmat
	    shift @{$ra_srladjmat->[$srl->[0]][$srl->[1]]};
	    if ($srl->[7]){ # lift
		print STDERR "? LIFT $srl->[0] $srl->[1] $srl->[2] $srl->[3] $srl->[10]\n" if ($debug);
		my $pplabel = $srl->[10] ? "$srl->[2]\*PB\*$srl->[3]" : "$srl->[2]\*NB\*$srl->[3]";
		if (_liftsrl($ra_srladjmat, $srl->[7], $srl->[1], $pplabel)){
		    push @ppsrls, [$srl->[7], $srl->[1], $pplabel];
		    print STDERR "LIFT $srl->[7] $srl->[1] $pplabel", "\n" if ($debug);
		    if ($srl->[10]){
			$nlifted_prop++;
		    }else{
			$nlifted_nom++;
		    }
		}else{
		    push @rmsrls, $srl;
		}
	    }elsif ($srl->[6] >= 0){ # loop
		print STDERR "? LOOP $srl->[0] $srl->[1] $srl->[2] $srl->[4] $srl->[10]\n"  if ($debug);
		my $pplabel = $srl->[10] ? "$srl->[4]\:PB\:$srl->[2]" : "$srl->[4]\:NB\:$srl->[2]";
		push @ppsrls, [$srl->[0], $srl->[0], $pplabel];
		print STDERR "LOOP $srl->[0] $srl->[0] $pplabel", "\n"  if ($debug);
		if (_liftsrl($ra_srladjmat, $srl->[0], $srl->[0], $pplabel)){
		    push @ppsrls, [$srl->[0], $srl->[0], $pplabel];
		    print STDERR "LOOP $srl->[0] $srl->[0] $pplabel", "\n" if ($debug);
		    if ($srl->[10]){
			$nlifted_prop++;
		    }else{
			$nlifted_nom++;
		    }
		}else{
		    push @rmsrls, $srl;
		}
	    }else{ # removal
		push @rmsrls, $srl;
	    }
	}# projective srl
    }
    foreach my $srl (@rmsrls){
	my $pplabel = $srl->[10] ? "$srl->[2]\*PB\*$srl->[3]" : "$srl->[2]\*NB\*$srl->[3]";
	if (_liftsrl($ra_srladjmat, $srl->[0], $srl->[1], $srl->[2])){
	    print STDERR "FALLBACK $srl->[0],$srl->[0] $srl->[1] $srl->[2]\n" if ($debug);
	}elsif (_liftsrl($ra_srladjmat, $srl->[7], $srl->[1], $pplabel)){
	    print STDERR "FALLBACK GOV LIFT $srl->[0],$srl->[7] $srl->[1] $pplabel\n" if ($debug);
	    if ($srl->[10]){
		$nlifted_prop++;
	    }else{
		$nlifted_nom++;
	    }
	}elsif (_liftsrl($ra_srladjmat, $srl->[11], $srl->[1], $pplabel)){
	    print STDERR "FALLBACK ROOT LIFT $srl->[0],$srl->[11] $srl->[1] $pplabel\n" if ($debug);
	    if ($srl->[10]){
		$nlifted_prop++;
	    }else{
		$nlifted_nom++;
	    }
	}else{
	    print STDERR "RM srl $srl->[0] $srl->[1] $srl->[2]\n"  if ($debug);
	    if ($srl->[10]){
		$nrm_prop++;
	    }else{
		$nrm_nom++;
	    }
	}
    }
    return [$nlifted_nom, $nrm_nom, $nlifted_prop, $nrm_prop];
}



sub isivanprojedge { # decides whether the input edge is projective or not
    my ($ra_adjmat, $ra_connv, $pred, $arg) = @_;
    return 1 if ($ra_adjmat->[$pred]->[$arg] == 0);
    my ($l, $u) = ($pred, $arg); 
    ($l, $u) = ($arg, $pred) if ($pred > $arg); 
    for my $k ($l+1..$u-1){
	if ($ra_connv->[$k] > 0){ 
	    for my $m (0..$l-1){
		if ($ra_adjmat->[$m][$k] || $ra_adjmat->[$k][$m]){
		    return 0;
		}
	    }
	    for my $m ($u+1..@{$ra_adjmat->[$conllf{'ID'}]}-1){
		if ($ra_adjmat->[$m][$k] || $ra_adjmat->[$k][$m]){
		    return 0;
		}
	    }
	}
    }
    return 1;
}

sub _ivannonprojdegree { 
# computes the non-projectivity degree of an edge 
# 0 if projective, 1 if crossing of exactly one edge, 2 if crossing of exactly two edges, ...
    my ($ra_adjmats, $pred, $arg) = @_;
    # return 0 if ($ra_adjmat->[$pred]->[$arg] == 0);
    my ($l, $u) = ($pred, $arg); 
    ($l, $u) = ($arg, $pred) if ($pred > $arg); 
    my $degree = 0;
    for my $k ($l+1..$u-1){
	if ($ra_adjmats->[4]->[$k]){ 
	# if ($ra_outdegconnv->[$k]){ 
	    for my $m (0..$l-1){
		if ($ra_adjmats->[0]->[$k][$m]){
		    $degree++; #  += @{$ra_adjmat->[$k][$m]};
		}
	    }
	    for my $m ($u+1..@{$ra_adjmats->[0]->[$conllf{'ID'}]}-1){
		if ($ra_adjmats->[0]->[$k][$m]){
		    $degree++; # += @{$ra_adjmat->[$k][$m]};
		}
	    }
	}
	if ($ra_adjmats->[3]->[$k]){ # in degree 
	    for my $m (0..$l-1){
		if ($ra_adjmats->[0]->[$m][$k]){
		    $degree++; # += @{$ra_adjmat->[$m][$k]};
		}
	    }
	    for my $m ($u+1..@{$ra_adjmats->[0]->[$conllf{'ID'}]}-1){
		if ($ra_adjmats->[0]->[$m][$k]){
		    $degree++; #  += @{$ra_adjmat->[$m][$k]};
		}
	    }
	}
    }
    return $degree; 
}

sub isivanproj($$){ # decides whether the input dependency graph has crossing edges or not 
    my ($ra_adjmat, $ra_connv) = @_;
    my $dim = @{$ra_adjmat->[$conllf{'ID'}]}-1;
    for my $i (0..$dim){
	for my $j (0..$dim){
	    if ($ra_adjmat->[$i][$j]){ 
		my ($l, $u) = ($i, $j); 
		($l, $u) = ($j, $i) if ($i > $j); 
		for my $k ($l+1..$u-1){
		    if ($ra_connv->[$k] > 0){ 
			for my $m (0..$l-1){
			    if ($ra_adjmat->[$m][$k] || $ra_adjmat->[$k][$m]){
				return 0;
			    }
			}
			for my $m ($u+1..$dim){
			    if ($ra_adjmat->[$m][$k] || $ra_adjmat->[$k][$m]){
				return 0;
			    }
			}
		    }
		}
	    }
	}
    }
    return 1;
}

sub ivanprojmat($$){ # compute the Ivan projectivity matrix according to which projective edges are parallel ones
    my ($ra_adjmat, $ra_connv) = @_;
    my (@projmat, @nonprojs, @projs);
    my $dim = @{$ra_adjmat->[$conllf{'ID'}]}-1;
    # my $loopy = 0; my $cyclic = 0;
    for my $i (0..$dim){
	for my $j (0..$dim){
	    if ($ra_adjmat->[$i][$j]){ 
		# $loopy = 1 if ($i == $j);
		# $cyclic = 1 if ($ra_adjmat->[$j][$i]);
		$projmat[$i][$j] = 1;
		my ($l, $u) = ($i, $j); 
		($l, $u) = ($j, $i) if ($i > $j); 
		for my $k ($l+1..$u-1){
		    if ($ra_connv->[$k] > 0){ 
			for my $m (0..$l-1){
			    if ($ra_adjmat->[$m][$k] || $ra_adjmat->[$k][$m]){
				$projmat[$i][$j] = -1; push @nonprojs, [$i, $j];
				last;
			    }
			}
			last if ($projmat[$i][$j] == -1);
			for my $m ($u+1..$dim){
			    if ($ra_adjmat->[$m][$k] || $ra_adjmat->[$k][$m]){
				$projmat[$i][$j] = -1; push @nonprojs, [$i, $j];
				last;
			    }
			}
			last if ($projmat[$i][$j] == -1);
		    }
		}
		push @projs, [$i,$j] if ($projmat[$i][$j] > 0);
	    }else{
		$projmat[$i][$j] = 0;
	    }
	}
    }
    return [\@projmat, \@nonprojs, \@projs]; # , $loopy, $cyclic];
}

sub ismarcusproj($$$){ # decides whether a dependency graph is projective or not according to the Marcus definition
    my ($ra_adjmat, $ra_closure, $ra_connv) = @_;
    my $dim = @{$ra_adjmat->[$conllf{'ID'}]}-1;
    for my $i (0..$dim){
	for my $j (0..$dim){
	    if ($ra_adjmat->[$i][$j]){ 
		my ($l, $u) = ($i, $j); 
		($l, $u) = ($j, $i) if ($i > $j); 
		for my $k ($l+1..$u-1){
		    if (($ra_connv->[$k] > 0) && !($ra_closure->[$i][$k])){ 
			# non-projective 
			return 0;
		    }
		}
	    }
	}
    }
    return 1;
}


sub marcusprojmat($$$){ # compute the Marcus projectivity matrix
    my ($ra_adjmat, $ra_closure, $ra_connv) = @_;
    my (@projmat, @nonprojs, @projs);
    @projmat = (); @nonprojs = (); @projs = ();
    my $dim = @{$ra_adjmat->[$conllf{'ID'}]}-1;
    my $loopy = 0; my $cyclic = 0;
    for my $i (0..$dim){
	for my $j (0..$dim){
	    if ($ra_adjmat->[$i][$j]){ 
		$loopy = 1 if ($i == $j);
		$cyclic = 1 if ($ra_closure->[$j][$i]);
		$projmat[$i][$j] = 1;
		my ($l, $u) = ($i, $j); 
		($l, $u) = ($j, $i) if ($i > $j); 
		for my $k ($l+1..$u-1){
		    if (($ra_connv->[$k] > 0) && !($ra_closure->[$i][$k])){ 
			# non-projective 
			$projmat[$i][$j] = -1;
			push @nonprojs, [$i, $j];
			last;
		    }
		}
		push @projs, [$i,$j] if ($projmat[$i][$j] > 0);
	    }else{
		$projmat[$i][$j] = 0;
	    }
	}
    }
    return [\@projmat, \@nonprojs, \@projs, $loopy, $cyclic];
}

sub closure_to_string($){
    # my ($ra_closure, $ra_connv) = @_;
    my $ra_closure = shift;;
    my $dim = @{$ra_closure}-1;
    # my $s = join(" ", @{$ra_connv}) . "\n\n";
    my $s = '';
    for my $i (0..$dim){
	$s .= "$i\t";
	for my $j (0..$dim){
	    $s .= $ra_closure->[$i]->[$j] . " ";
	    
	}
	$s .= "\n";
    }
    return $s . "\n";
}

sub closurerec_to_array {
    my $closurec = shift;
    my @rows = split /\n/, $closurec;
    my @closure;
    foreach my $row (@rows){
	my @fields = split /\s+/, $row;
	push @closure, [@fields[1..@fields-1]];
    }
    
    return \@closure;
}

sub closure($){
    my $ra_adjmat = shift;
    my $dim = @{$ra_adjmat->[$conllf{'ID'}]}-1;
    my @closure; 
    for my $i (0..$dim){
	for my $j (0..$dim){
	    $closure[$i][$j] = 0;
	    if ($ra_adjmat->[$i]->[$j]){
		$closure[$i][$j] = 1;
	    }
	}
    }
    for my $k (0..$dim){
	for my $i (0..$dim){
	    for my $j (0..$dim){
		# $closure[$i][$j] = ($closure[$i][$k] && $closure[$k][$j]) if (!$closure[$i][$j]);
		if (!$closure[$i][$j] && ($closure[$i][$k] && $closure[$k][$j])){
		    $closure[$i][$j] = $closure[$i][$k] + $closure[$k][$j];
		}
	    }
	}
    }
    return \@closure;
}



sub adjmat_to_string($){
    my $ra_adjmat = shift;
    my $dim = @{$ra_adjmat}-1;
    my $s = '';
    for my $i (0..$dim){
	for my $j (0..$dim){
	    if (defined($ra_adjmat->[$i][$j]) && $ra_adjmat->[$i][$j]){
		$s .= "{" . join(",", @{$ra_adjmat->[$i][$j]}) . "} ";
	    }else{
		$s .= ($j == 0) ? "$i\t" : "$j ";
	    }
	}
	$s .= "\n";
    }
    return $s . "\n";
}

sub rara_to_string {
    my $rara = shift;
    my $s = '';
    foreach my $ra (@{$rara}){
	$s .= join(",", @{$ra});
	$s .= "\n";
    }
    return $s . "\n";
}

sub srl_confusionm {
    my ($ra_srladjmat_g, $ra_srladjmat_s, $rh_confm, $rh_srls) = @_;
    croak "unequal srl_adjmat dimensions" if (@{$ra_srladjmat_g} != @{$ra_srladjmat_s});
    if (!exists($rh_srls->{'null'})){
	$rh_srls->{'null'} = 1;
    }
    foreach my $i (1..@{$ra_srladjmat_g}-1){
	foreach my $j (1..@{$ra_srladjmat_g->[$i]}-1){
	    my $srl_g = $ra_srladjmat_g->[$i]->[$j] ? $ra_srladjmat_g->[$i]->[$j]->[0] : 'null';
	    my $srl_s = $ra_srladjmat_s->[$i]->[$j] ? $ra_srladjmat_s->[$i]->[$j]->[0] : 'null';
	    $rh_confm->{$srl_g}->{$srl_s}++;
	    $rh_srls->{$srl_g} = 1; $rh_srls->{$srl_s} = 1;
	}
    }
    foreach my $srl_g (keys %{$rh_srls}){
	foreach my $srl_s (keys %{$rh_srls}){
	    $rh_confm->{$srl_g}->{$srl_s} = 0 if (!exists($rh_confm->{$srl_g}->{$srl_s}));
	}
    }
}
sub confm_to_f1_string {
    my $rh_confm = shift;
    my $s;
    my (%prec, %rec);
    foreach my $srlr (keys %{$rh_confm}){
	foreach my $srlc (keys %{$rh_confm->{$srlr}}){
	    $prec{$srlr} += $rh_confm->{$srlc}->{$srlr}; 
	    $rec{$srlc} += $rh_confm->{$srlc}->{$srlr}; 
	}
    }
    foreach my $srl (sort keys %prec){
	my $num = $rh_confm->{$srl}->{$srl};
	my $prec = ($prec{$srl} > 0) ? $num/($prec{$srl})*100 : "-1";
	my $rec = ($rec{$srl} > 0) ? $num/($rec{$srl})*100 : "-1";
	my $f1 = (($prec + $rec) > 0)? (2*$prec*$rec)/($prec+$rec): "-1";
	$s .= sprintf("%-13s: %-3.2f %-3.2f %-3.2f   %-13d %-13d %-13d\n", $srl, $prec, $rec, $f1, $num, $prec{$srl}, $rec{$srl});
    }
    return $s;
}
sub confm_to_string {
    my $rh_confm = shift;
    my $s = ""; my $i = 0;
    my @a_sorted = sort keys %{$rh_confm};
    my %h_prec; my %h_rec;
    foreach my $srl_g (@a_sorted){
	$s .= sprintf("%2d: %-8s ", $i++, $srl_g);
	foreach my $srl_s (@a_sorted){
	    $s .= sprintf("%5d ", $rh_confm->{$srl_g}->{$srl_s});
	}
	$s .= sprintf("\n");
    }
    return $s;
}

# &cplanarityiodegree &is_cprojective &is_cplanar &cplanaritydegree &csemdgclosure &csemdg

sub cplanarityiodegree {
# computes the planarity ingoing and outgoing degree of a graph 
    my ($ra_adjmat, $ra_indegree, $ra_outdegree, $ra_edgedegree, $ra_edgeindegree, $ra_edgeoutdegree, $debug) = @_;
    my $dim = @{$ra_adjmat->[$conllf{'ID'}]}-1;
    my $maxdegree = 0;
    my $maxindegree = 0;
    my $maxoutdegree = 0;
    my $degree = 0; my $indegree = 0; my $outdegree = 0;
    my $edge = 0; my $npedge = 0;
    for my $i (0..$dim){
	for my $j (0..$dim){
	    next if (!($ra_adjmat->[$i][$j]));
	    my $ra_degrees = _cplanarityiodegree($ra_adjmat, $ra_indegree, $ra_outdegree, $i, $j, $debug);
	    $indegree = $ra_degrees->[0];
	    $outdegree = $ra_degrees->[1];
	    $degree = $ra_degrees->[2];
	    $ra_edgedegree->[$degree]++;
	    $ra_edgeindegree->[$indegree]++;
	    $ra_edgeoutdegree->[$outdegree]++;
	    print STDERR "cplanarityinoutdegree $i,$j $indegree $outdegree $degree\n" if ($debug);
	    $maxdegree = ($degree > $maxdegree) ? $degree : $maxdegree;
	    $maxindegree = ($indegree > $maxindegree) ? $indegree : $maxindegree;
	    $maxindegree = ($outdegree > $maxoutdegree) ? $outdegree : $maxoutdegree;
	    $edge++; $npedge++ if (!$degree);
	}
    }
    return [$maxindegree, $maxoutdegree, $maxdegree, $edge, $npedge];
}

sub _cplanarityiodegree {
# computes the planarity ingoing and outgoing degree of an edge 
    my ($ra_adjmat, $ra_indegree, $ra_outdegree, $pred, $arg, $debug) = @_;
    my ($l, $u) = ($pred, $arg); 
    ($l, $u) = ($arg, $pred) if ($pred > $arg); 
    my $degree = 0; my $indegree = 0; my $outdegree = 0;
    for my $k ($l+1..$u-1){
	if ($ra_outdegree->[$k]){  # out degree
	    for my $m (0..$l-1){
		if ($ra_adjmat->[$k][$m]){
		    $degree++;
		    $outdegree++;
		}
	    }
	    for my $m ($u+1..@{$ra_adjmat->[$conllf{'ID'}]}-1){
		if ($ra_adjmat->[$k][$m]){
		    $degree++;
		    $outdegree++;
		}
	    }
	}
	if ($ra_indegree->[$k]){ # in degree 
	    for my $m (0..$l-1){
		if ($ra_adjmat->[$m][$k]){
		    $degree++;
		    $indegree++;
		}
	    }
	    for my $m ($u+1..@{$ra_adjmat->[$conllf{'ID'}]}-1){
		if ($ra_adjmat->[$m][$k]){
		    $degree++;
		    $indegree++;
		}
	    }
	}
    }
    return [$indegree, $outdegree, $degree];    
}

sub cplanaritydegree {
    my ($ra_adjmat, $ra_indegree, $ra_outdegree, $ra_edgedegree, $debug) = @_;
    my $dim = @{$ra_adjmat->[$conllf{'ID'}]}-1;
    my $maxdegree = 0;
    for my $i (0..$dim){
	for my $j (0..$dim){
	    next if (!($ra_adjmat->[$i][$j]));
	    my $degree = _cplanaritydegree($ra_adjmat, $ra_indegree, $ra_outdegree, $i, $j, $debug);
	    $ra_edgedegree->[$degree]++;
	    print STDERR "cplanaritydegree $i,$j $degree\n" if ($debug);
	    $maxdegree = ($degree > $maxdegree) ? $degree : $maxdegree;
	}
    }
    return $maxdegree;
}

sub _cplanaritydegree {
# computes the planarity degree of an edge 
    my ($ra_adjmat, $ra_indegree, $ra_outdegree, $pred, $arg, $debug) = @_;
    return 0 if (!$ra_adjmat->[$pred][$arg]);
    my ($l, $u) = ($pred, $arg); 
    ($l, $u) = ($arg, $pred) if ($pred > $arg); 
    my $degree = 0;
    for my $k ($l+1..$u-1){
	if ($ra_outdegree->[$k]){ 
	    for my $m (0..$l-1){
		if ($ra_adjmat->[$k][$m]){
		    $degree++;
		    print STDERR "in left $pred, $arg\n" if ($debug);
		}
	    }
	    for my $m ($u+1..@{$ra_adjmat->[$conllf{'ID'}]}-1){
		if ($ra_adjmat->[$k][$m]){
		    $degree++;
		    print STDERR "in right $pred, $arg\n" if ($debug);
		}
	    }
	}
	if ($ra_indegree->[$k]){ # in degree 
	    for my $m (0..$l-1){
		if ($ra_adjmat->[$m][$k]){
		    $degree++;
		    print STDERR "out left $pred, $arg\n" if ($debug);
		}
	    }
	    for my $m ($u+1..@{$ra_adjmat->[$conllf{'ID'}]}-1){
		if ($ra_adjmat->[$m][$k]){
		    $degree++;
		    print STDERR "out right $pred, $arg\n" if ($debug);
		}
	    }
	}
    }
    return $degree;    
}

sub is_cplanar {
    my $ra_adjmat = shift;
    my $dim = @{$ra_adjmat->[$conllf{'ID'}]}-1;

    my $edge  = 0;
    my $npedge = 0;

    for my $i (0..$dim){
	for my $j (0..$dim){
	    if ($ra_adjmat->[$i][$j]){ 
		$edge++;
		$npedge++ if (!_cplanar($ra_adjmat, $i, $j));
	    }
	}
    }
    return [$edge, $npedge];
    # return 1;
}

sub nonplanarminsubg { # returns the minimal nonplanar sugbraphs, ie. the set of pairs of crossing links
    my ($ra_mat, $rn) = @_;
    my (%lnpset, %unpset, %lset, %uset);
    %lnpset = (); %unpset = (); %lset = (); %uset = ();
    for my $i (0..@{$ra_mat}-1){
	for my $j (0..@{$ra_mat}-1){
	    next if (!defined($ra_mat->[$i][$j]) or !@{$ra_mat->[$i][$j]});
	    my $lij = $ra_mat->[$i][$j]->[0];
	    if ($i == $j){
		$lset{$rn.':'.$i.','.$lij.','.$j} = 1;
		$uset{$rn.':'.$i.','.$j} = 1;
		next;
	    }
	    my ($l, $u) = ($i, $j); 
	    ($l, $u) = ($j, $i) if ($i > $j);
	    my $np = 0;
	    for my $k ($l+1..$u-1){
		for my $m (0..$l-1){ # $m < $k
		    if (@{$ra_mat->[$m][$k]}){
			my $lmk = $ra_mat->[$m][$k]->[0];
			# $basenpset{$rn.':'.$i.','.$lij.','.$j}++;
			# $basenpset{$rn.':'.$m.','.$lmk.','.$k}++;			
			if ($i < $m){
			    $lnpset{$rn.':'.$i.','.$lij.','.$j.';'.$m.','.$lmk.','.$k}++;
			    $unpset{$rn.':'.$i.','.$j.';'.$m.','.$k}++;
			}else{
			    $lnpset{$rn.':'.$m.','.$lmk.','.$k.';'.$i.','.$lij.','.$j}++;
			    $unpset{$rn.':'.$m.','.$k.';'.$i.','.$j}++;
			}
			$np = 1;
		    }
		    if (@{$ra_mat->[$k][$m]}){
			my $lkm = $ra_mat->[$k][$m]->[0];
			# $basenpset{$rn.':'.$i.','.$lij.','.$j}++;
			# $basenpset{$rn.':'.$k.','.$lkm.','.$m}++;
			if ($i < $k){
			    $lnpset{$rn.':'.$i.','.$lij.','.$j.';'.$k.','.$lkm.','.$m}++;
			    $unpset{$rn.':'.$i.','.$j.';'.$k.','.$m}++;
			}else{
			    $lnpset{$rn.':'.$k.','.$lkm.','.$m.';'.$i.','.$lij.','.$j}++;
			    $unpset{$rn.':'.$k.','.$m.';'.$i.','.$j}++;
			}
			$np = 1;
		    }
		}
		for my $m ($u+1..@{$ra_mat}-1){ # $m > $k
		    if (@{$ra_mat->[$m][$k]}){
			my $lmk = $ra_mat->[$m][$k]->[0];
			# $basenpset{$rn.':'.$i.','.$lij.','.$j}++;
			# $basenpset{$rn.':'.$m.','.$lmk.','.$k}++;
			if ($i < $m){
			    $lnpset{$rn.':'.$i.','.$lij.','.$j.';'.$m.','.$lmk.','.$k}++;
			    $unpset{$rn.':'.$i.','.$j.';'.$m.','.$k}++;
			}else{
			    $lnpset{$rn.':'.$m.','.$lmk.','.$k.';'.$i.','.$lij.','.$j}++;
			    $unpset{$rn.':'.$m.','.$k.';'.$i.','.$j}++;
			}
			$np = 1;
		    }
		    if (@{$ra_mat->[$k][$m]}){
			my $lkm = $ra_mat->[$k][$m]->[0];
			# $basenpset{$rn.':'.$i.','.$lij.','.$j}++;
			# $basenpset{$rn.':'.$k.','.$lkm.','.$m}++;
			if ($i < $k){
			    $lnpset{$rn.':'.$i.','.$lij.','.$j.';'.$k.','.$lkm.','.$m}++;
			    $unpset{$rn.':'.$i.','.$j.';'.$k.','.$m}++;
			}else{
			    $lnpset{$rn.':'.$k.','.$lkm.','.$m.';'.$i.','.$lij.','.$j}++;
			    $unpset{$rn.':'.$k.','.$m.';'.$i.','.$j}++;
			}
			$np = 1;
		    }
		}
	    }
	    if ($np){
		$lset{$rn.':'.$i.','.$lij.','.$j}--;
		$uset{$rn.':'.$i.','.$j.';'}--;
	    }else{
		$lset{$rn.':'.$i.','.$lij.','.$j}++;
		$uset{$rn.':'.$i.','.$j}++;
	    }
	}
    }
    # %lnpset set of minimal nonplanar subgraphs (ie. set of pairs of crossing links)
    # %unpset set of unlabeled minimal nonplanar sugbraphs (ie. set of pairs of crossing links)
    # %lset set of links (planar(== 1) or not(== -1))
    # %uset set of unlabeled links (planar(== 1) or not(== -1))
    return [\%lnpset, \%unpset, \%lset, \%uset];
}

sub _cplanar {
    my ($ra_adjmat, $pred, $arg) = @_;
    return 1 if ($pred == $arg);
    my ($l, $u) = ($pred, $arg); 
    ($l, $u) = ($arg, $pred) if ($pred > $arg); 
    for my $k ($l+1..$u-1){
	for my $m (0..$l-1){
	    if ($ra_adjmat->[$m][$k] || $ra_adjmat->[$k][$m]){
		return 0;
	    }
	}
	for my $m ($u+1..@{$ra_adjmat->[$conllf{'ID'}]}-1){
	    if ($ra_adjmat->[$m][$k] || $ra_adjmat->[$k][$m]){
		return 0;
	    }
	}
    }
    return 1;
}

sub is_cprojective {
    my ($ra_adjmat, $ra_closure, $ra_indegree, $ra_outdegree) = @_;
    my $dim = @{$ra_adjmat->[$conllf{'ID'}]}-1;
    my $npedge = 0; my $edge = 0;
    my $np = 0; my $planar = 1;
    for my $i (0..$dim){
	for my $j (0..$dim){
	    if ($ra_adjmat->[$i][$j]){
		
		$edge++;
		
		my ($l, $u) = ($i, $j); 
		($l, $u) = ($j, $i) if ($i > $j); 
		
		$planar = _cplanar($ra_adjmat, $i, $j);
		
		if (!$planar){
		    $np++; next;
		}
		
		for my $k ($l+1..$u-1){
		    
		    if (($ra_indegree->[$k] || $ra_outdegree->[$k]) && !($ra_closure->[$i][$k])){ 
			# non-projective 
		
			$npedge = 1;
			last;

                        # return 0;
		    }
		}

		$np++ if ($npedge);

	    }
	}
    }

    return [$edge, $np];

    # return 1;
}

sub csemdgclosure {
    my $ra_adjmat = shift;
    my $dim = @{$ra_adjmat->[$conllf{'ID'}]}-1;
    my @closure; 
    for my $i (0..$dim){
	for my $j (0..$dim){
	    $closure[$i][$j] = 0;
	    if ($ra_adjmat->[$i]->[$j]){
		$closure[$i][$j] = 1;
	    }
	}
    }
    for my $k (0..$dim){
	for my $i (0..$dim){
	    for my $j (0..$dim){
		$closure[$i][$j] = ($closure[$i][$k] && $closure[$k][$j]) if (!$closure[$i][$j]);
	    }
	}
    }
    return \@closure;
}

sub csyntsemdgclosure {
    my $ra_adjmat = shift;
    my $dim = @{$ra_adjmat->[$conllf{'ID'}]}-1;
    my @closure; 
    for my $i (0..$dim){
	for my $j (0..$dim){
	    $closure[$i][$j] = 0;
	    if ($ra_adjmat->[$i]->[$j]){
		$closure[$i][$j] = 1;
	    }
	}
    }
    for my $k (0..$dim){
	for my $i (0..$dim){
	    for my $j (0..$dim){
		$closure[$i][$j] = ($closure[$i][$k] && $closure[$k][$j]) if (!$closure[$i][$j]);
	    }
	}
    }
    return \@closure;
}


sub csyntsemdg { # compute the union of the syntactic and semantic graphs, that might result in a multi-graph
    my $ra_conll = shift;
    my (@adjmat, @indegree, @outdegree, @pindexes);
    my $dim = @{$ra_conll->[$conllf{'ID'}]}-1;
    for my $i (0..$dim){
	$indegree[$i] = 0 if (!defined($indegree[$i]));
	$outdegree[$i] = 0 if (!defined($outdegree[$i]));
	if (($ra_conll->[$conllf{'PRED'}]->[$i] ne '_') && ($ra_conll->[$conllf{'PRED'}]->[$i] ne '-1')){
	    push @pindexes, $i;
	}
	if (@pindexes && ($pindexes[@pindexes-1] == $i)){
	    for my $j (0..$dim){
		if (($ra_conll->[$conllf{'ARG'}+@pindexes-1]->[$j] ne '_') && 
		    ($ra_conll->[$conllf{'ARG'}+@pindexes-1]->[$j] ne '-1')){
		    ++$outdegree[$i]; ++$indegree[$j];
		    $adjmat[$i][$j]++;
		}else{
		    $adjmat[$i][$j] = 0;
		}
	    }
	}else{
	    for my $j (0..$dim){
		$adjmat[$i][$j] = 0;
	    }
	}
    }
    for my $i (0..$dim){
	for my $j (0..$dim){
	    if ($i == $ra_conll->[$conllf{'HEAD'}]->[$j]){
		++$outdegree[$i]; ++$indegree[$j];
		$adjmat[$i][$j]++;
	    }else{
		$adjmat[$i][$j] = 0;
	    }
	}
    }
    return [\@adjmat, \@indegree, \@outdegree];
}

sub csemdg {
    my $ra_conll = shift;
    my (@adjmat, @indegree, @outdegree, @pindexes);
    my $dim = @{$ra_conll->[$conllf{'ID'}]}-1;
    for my $i (0..$dim){
	$indegree[$i] = 0 if (!defined($indegree[$i]));
	$outdegree[$i] = 0 if (!defined($outdegree[$i]));
	if (($ra_conll->[$conllf{'PRED'}]->[$i] ne '_') && ($ra_conll->[$conllf{'PRED'}]->[$i] ne '-1')){
	    push @pindexes, $i;
	}
	if (@pindexes && ($pindexes[@pindexes-1] == $i)){
	    for my $j (0..$dim){
		if (($ra_conll->[$conllf{'ARG'}+@pindexes-1]->[$j] ne '_') && 
		    ($ra_conll->[$conllf{'ARG'}+@pindexes-1]->[$j] ne '-1')){
		    ++$outdegree[$i]; ++$indegree[$j];
		    $adjmat[$i][$j]++;
		}else{
		    $adjmat[$i][$j] = 0;
		}
	    }
	}else{
	    for my $j (0..$dim){
		$adjmat[$i][$j] = 0;
	    }
	}
    }
    return [\@adjmat, \@indegree, \@outdegree];
}

sub srl_adjmats{ # returns the labeled PropBank and NomBank dependency matrices
    my $ra_conll = shift;
    my (@srl_adjmat, @nb_adjmat, @pb_adjmat, @indegconnv, @outdegconnv); 
    # $connv[$i] is > 0 if $i is a srl dependent or a srl governor, $connv[$i] is the sum of the in and out SRL degrees
    my (@pindexes, @pb_preds, @nb_preds, @pbsrls, @nbsrls, %nbsrlfreq, %pbsrlfreq, %ivannbsrlfreq, %ivanpbsrlfreq);
    my $dim = @{$ra_conll->[$conllf{'ID'}]}-1;
    my $propbank;
    my $ivanbank = -1;
    for my $i (0..$dim){
	$indegconnv[$i] = 0 if (!defined($indegconnv[$i]));
	$outdegconnv[$i] = 0 if (!defined($outdegconnv[$i]));
	if (($ra_conll->[$conllf{'PRED'}]->[$i] ne '_') && ($ra_conll->[$conllf{'PRED'}]->[$i] ne '-1')){
	    push @pindexes, $i;
	}
	if (@pindexes && ($pindexes[@pindexes-1] == $i)){
	    if ($ra_conll->[$conllf{'GPOS'}]->[$i] =~ /^VB/){
		push @pb_preds, [$i, $ra_conll->[$conllf{'PRED'}]->[$i]];
		$propbank = 1;
	    }else{
		push @nb_preds, [$i, $ra_conll->[$conllf{'PRED'}]->[$i]];
		$propbank = 0;
	    }
	    # set the SENSE feature
	    if (($ra_conll->[$conllf{'PPOSS'}]->[$i] =~ /^NN/) &&
		($ra_conll->[$conllf{'GPOS'}]->[$i] =~ /^NN/)){
		$ivanbank = 1;
	    }elsif (($ra_conll->[$conllf{'PPOSS'}]->[$i] =~ /^VB/) &&
		    ($ra_conll->[$conllf{'GPOS'}]->[$i] =~ /^VB/)){
		$ivanbank = 0;
	    }elsif ($ra_conll->[$conllf{'PPOSS'}]->[$i] eq $ra_conll->[$conllf{'GPOS'}]->[$i]){
		$ivanbank = 0;
	    }else{
		$ivanbank = -1;
	    }
	    for my $j (0..$dim){
		if (($ra_conll->[$conllf{'ARG'}+@pindexes-1]->[$j] ne '_') && 
		    ($ra_conll->[$conllf{'ARG'}+@pindexes-1]->[$j] ne '-1')){
		    my $srl = $ra_conll->[$conllf{'ARG'}+@pindexes-1]->[$j];
		    my $pred = $ra_conll->[$conllf{'PRED'}]->[$i];
		    ++$outdegconnv[$i]; ++$indegconnv[$j];
		    push @{$srl_adjmat[$i][$j]}, $srl;
		    if ($propbank){
			push @{$pb_adjmat[$i][$j]}, $srl;
			$nb_adjmat[$i][$j] = 0;
			push @pbsrls, [$i, $j, $srl]; # pred index, arg index, semantic role
			$pbsrlfreq{$srl}++;
		    }else{
			push @{$nb_adjmat[$i][$j]}, $srl;
			$pb_adjmat[$i][$j] = 0;
			push @nbsrls, [$i, $j, $srl]; # pred index, arg index, semantic role
			$nbsrlfreq{$srl}++;
		    }
		    if ($ivanbank == 1){
			$ivannbsrlfreq{$srl}++;
		    }elsif ($ivanbank == 0){
			$ivanpbsrlfreq{$srl}++;
		    }
		}else{
		    $srl_adjmat[$i][$j] = 0;
		    $pb_adjmat[$i][$j] = 0;
		    $nb_adjmat[$i][$j] = 0;
		}
	    }
	}else{
	    for my $j (0..$dim){
		$srl_adjmat[$i][$j] = 0;
		$pb_adjmat[$i][$j] = 0;
		$nb_adjmat[$i][$j] = 0;
	    }
	}
    }
    return [\@srl_adjmat, \@pb_adjmat, \@nb_adjmat, 
	    \@indegconnv, \@outdegconnv, 
	    \@pb_preds, \@nb_preds, \@pbsrls, \@nbsrls, 
	    \%pbsrlfreq, \%nbsrlfreq, \%ivanpbsrlfreq, \%ivannbsrlfreq]; 
    # @srl_adjmat stores the union of @pb_adjmat and @nb_adjmat
    # @pbsrls store PropBank [predindex, argindex, srl]
    # @pbsrls store NomBank [predindex, argindex, srl]
}

sub connectedsrladjmat {
    my $ra_conll = shift;
    my (@srl_adjmat, @a_inedge);
    my $dim = @{$ra_conll->[$conllf{'ID'}]}-1;
    @a_inedge = (0) x (@{$ra_conll->[$conllf{'ID'}]});
    my $k = 0;
    for my $i (0..$dim){
	if (($ra_conll->[$conllf{'PRED'}]->[$i] ne '_') && ($ra_conll->[$conllf{'PRED'}]->[$i] ne '-1')){
	    my $p = $ra_conll->[$conllf{'PRED'}]->[$i];
	    my ($lemma, $sense) = split /\./, $p;
	    push @{$srl_adjmat[0][$i]}, $sense;
	    $a_inedge[$i]++;
	    $k++;
	    foreach my $j (0..$dim){
		if (($ra_conll->[$conllf{'PRED'}+$k]->[$j] ne '_') && 
		    ($ra_conll->[$conllf{'PRED'}+$k]->[$j] ne '-1')){
		    my $srl = $ra_conll->[$conllf{'PRED'}+$k]->[$j];
		    push @{$srl_adjmat[$i][$j]}, $srl;
		    confess "-1 $i $j" if (!defined($srl_adjmat[0][$i]) or !@{$srl_adjmat[0][$i]});
		    if ($i != $j){
			$a_inedge[$j]++;
		    }
		}else{
		    $srl_adjmat[$i][$j] = [];
		}
	    }
	}else{
	    foreach my $j (0..$dim){
		$srl_adjmat[$i][$j] = [];
	    }
	}
    }
    my $parallel = 0;
    for my $i (0..$dim){
	foreach my $j (0..$dim){
	    $parallel++ if (($i != $j) and 
			    defined($srl_adjmat[$j][$i]) and @{$srl_adjmat[$j][$i]} 
			    and defined($srl_adjmat[$i][$j]) and @{$srl_adjmat[$i][$j]});
	}
    }
	
    my ($v, $e);
    $v = 0; $e = 0;
    map {$e += $_ } @a_inedge;
    map {$v++ if ($_ > 0) } @a_inedge;
    # print STDERR "\n", join(" ", @a_inedge), " --", scalar(@a_inedge), "\n\n";
    $v++; # counts the root
    # print STDERR "-- $parallel $v $e", "\n";
    # confess "-1 $parallel $v $e" if ($parallel);
    $parallel /= 2;
    return [\@srl_adjmat, $v, $e-$parallel];
}

sub depsrl_adjmats { # label dependency tree matric and labeled semantic graph matrix
    my $ra_conll = shift;
    my (@dep_adjmat, %deplink, @srl_adjmat, %srllink);
    my $dim = @{$ra_conll->[$conllf{'ID'}]}-1;
    my $k = 0;
    for my $i (0..$dim){
	if (($ra_conll->[$conllf{'PRED'}]->[$i] ne '_') && ($ra_conll->[$conllf{'PRED'}]->[$i] ne '-1')){
	    $k++;
	    foreach my $j (0..$dim){
		if (($ra_conll->[$conllf{'PRED'}+$k]->[$j] ne '_') && 
		    ($ra_conll->[$conllf{'PRED'}+$k]->[$j] ne '-1')){
		    my $srl = $ra_conll->[$conllf{'PRED'}+$k]->[$j];
		    push @{$srl_adjmat[$i][$j]}, $srl;
		}else{
		    $srl_adjmat[$i][$j] = [];
		}
	    }
	}else{
	    foreach my $j (0..$dim){
		$srl_adjmat[$i][$j] = [];
	    }
	}
    }
    for my $i (0..$dim){
	for my $j (0..$dim){
	    if ($i == $ra_conll->[$conllf{'HEAD'}]->[$j]){
		push @{$dep_adjmat[$i][$j]}, $ra_conll->[$conllf{'DEPREL'}]->[$j];
	    }else{
		$dep_adjmat[$i][$j] = [];
	    }
	}
    }
    return [\@dep_adjmat, \@srl_adjmat]; 
}

sub pr {
    my ($ra_gold, $ra_model) = @_;
    my $dim = @{$ra_gold->[$conllf{'ID'}]}-1;
    confess "-1" if ($dim != (@{$ra_gold->[$conllf{'ID'}]}-1));
    my $p = 0; my $r = 0; my $t = 0;
    for my $i (0..$dim){
	for my $j (0..$dim){
	    if (defined($ra_gold->[$i][$j]) and @{$ra_gold->[$i][$j]}){
		$r++;
	    }
	    if (defined($ra_model->[$i][$j]) and @{$ra_model->[$i][$j]}){
		$p++;
	    }
	    if (defined($ra_gold->[$i][$j]) and @{$ra_gold->[$i][$j]} and 
		defined($ra_model->[$i][$j]) and @{$ra_model->[$i][$j]} and 
		($ra_gold->[$i][$j]->[0] eq $ra_model->[$i][$j]->[0])){
		$t++;
		confess "-1" if (@{$ra_gold->[$i][$j]} > 1);
		confess "-1" if (@{$ra_model->[$i][$j]} > 1);
	    }
	}
    }
    return [$t, $p, $r];
}

sub ss_adjmat {
    my $ra_conll = shift;
    my @adjmat = ();
    my $dim = @{$ra_conll->[$conllf{'ID'}]}-1;
    my $k = 0;
    for my $i (0..$dim){
	if (($ra_conll->[$conllf{'PRED'}]->[$i] ne '_') && ($ra_conll->[$conllf{'PRED'}]->[$i] ne '-1')){
	    my ($lemma, $sense) = split /\./, $ra_conll->[$conllf{'PRED'}]->[$i];
	    if ($sense =~ /^\d$/){
		$sense = '0' . $sense; # ciaramita
	    }
	    push @{$adjmat[0][$i]}, $sense;
	    $k++;
	    foreach my $j (0..$dim){
		if (($ra_conll->[$conllf{'PRED'}+$k]->[$j] ne '_') && 
		    ($ra_conll->[$conllf{'PRED'}+$k]->[$j] ne '-1')){
		    my $srl = $ra_conll->[$conllf{'PRED'}+$k]->[$j];
		    push @{$adjmat[$i][$j]}, $srl;
		}else{
		    $adjmat[$i][$j] = [];
		}
	    }
	}else{
	    foreach my $j (0..$dim){
		$adjmat[$i][$j] = [];
	    }
	}
    }
    return \@adjmat;
}

sub sx_adjmat {
    my $ra_conll = shift;
    my @adjmat = ();
    my $dim = @{$ra_conll->[$conllf{'ID'}]}-1;
    for my $i (0..$dim){
	for my $j (0..$dim){
	    if ($i == $ra_conll->[$conllf{'HEAD'}]->[$j]){
		push @{$adjmat[$i][$j]}, $ra_conll->[$conllf{'DEPREL'}]->[$j];
	    }else{
		$adjmat[$i][$j] = [];
	    }
	}
    }
    return \@adjmat;
}

sub srladjmats($){ # returns the labeled semantic dependency matrices
    my $ra_conll = shift;
    my (@srl_adjmat, @nb_adjmat, @pb_adjmat, @connv); 
    # $connv[$i] is > 0 if $i is a srl dependent or a srl governor, $connv[$i] is the sum of the in and out SRL degrees
    my (@pindexes, @pb_preds, @nb_preds, @pbsrls, @nbsrls);
    @pbsrls = (); @nbsrls = ();
    my $dim = @{$ra_conll->[$conllf{'ID'}]}-1;
    for my $i (0..$dim){
	$connv[$i] = 0 if (!defined($connv[$i]));
	if (($ra_conll->[$conllf{'PRED'}]->[$i] ne '_') && ($ra_conll->[$conllf{'PRED'}]->[$i] ne '-1')){
	    push @pindexes, $i;
	}
	if (@pindexes && ($pindexes[@pindexes-1] == $i)){
	    if ($ra_conll->[$conllf{'GPOS'}]->[$i] =~ /^VB/){
		push @pb_preds, $ra_conll->[$conllf{'PRED'}]->[$i];
	    }else{
		push @nb_preds, $ra_conll->[$conllf{'PRED'}]->[$i];
	    }
	    for my $j (0..$dim){
		if (($ra_conll->[$conllf{'ARG'}+@pindexes-1]->[$j] ne '_') && 
		    ($ra_conll->[$conllf{'ARG'}+@pindexes-1]->[$j] ne '-1')){
		    my $srl = $ra_conll->[$conllf{'ARG'}+@pindexes-1]->[$j];
		    my $pred = $ra_conll->[$conllf{'PRED'}]->[$i];
		    ++$connv[$i]; ++$connv[$j];
		    if ($ra_conll->[$conllf{'GPOS'}]->[$i] =~ /^VB/){
			push @{$pb_adjmat[$i][$j]}, $srl.':'.$pred;
			push @{$srl_adjmat[$i][$j]}, $srl.':'.$pred;
			$nb_adjmat[$i][$j] = 0;
			push @pbsrls, [$j, $i, $srl]; # arg index, pred index, semantic role
		    }else{
			push @{$nb_adjmat[$i][$j]}, $srl.'*'.$pred;
			push @{$srl_adjmat[$i][$j]}, $srl.'*'.$pred;
			$pb_adjmat[$i][$j] = 0;
			push @nbsrls, [$j, $i, $srl]; # arg index, pred index, semantic role
		    }
		    
		}else{
		    $srl_adjmat[$i][$j] = 0;
		    $pb_adjmat[$i][$j] = 0;
		    $nb_adjmat[$i][$j] = 0;
		}
	    }
	}else{
	    for my $j (0..$dim){
		$srl_adjmat[$i][$j] = 0;
		$pb_adjmat[$i][$j] = 0;
		$nb_adjmat[$i][$j] = 0;
	    }
	}
    }
    return [\@srl_adjmat, \@pb_adjmat, \@nb_adjmat, 
	    \@connv, \@pb_preds, \@nb_preds, 
	    \@pbsrls, \@nbsrls]; 
    # @srl_adjmat stores the union of @pb_adjmat and @nb_adjmat
    # @pbsrls store PropBank [argindex, predindex, srl]
    # @pbsrls store NomBank [argindex, predindex, srl]
}

sub syntadjmat($){ # returns the labeled syntactic dependency adjacency matrix
    my ($ra_conll) = @_;
    my (@adjmat, @connv, @adjlist); # $connv[$i] is 1 if $i is a dependent or a governor 
    my $dim = @{$ra_conll->[$conllf{'ID'}]}-1;
    for my $i (0..$dim){
	$connv[$i] = 0 if (!defined($connv[$i]));
	for my $j (0..$dim){
	    if ($i == $ra_conll->[$conllf{'HEAD'}]->[$j]){
		push @{$adjmat[$i][$j]}, $ra_conll->[$conllf{'DEPREL'}]->[$j];
		push @{$adjlist[$i]}, [$j, $ra_conll->[$conllf{'DEPREL'}]->[$j]]; # adjacency list
		++$connv[$i]; ++$connv[$j];
	    }else{
		$adjmat[$i][$j] = 0;
	    }
	}
    }
    return [\@adjmat, \@connv, \@adjlist];
}

sub conllarray_to_srlstring {
    my $ra_array = shift;
    my $s = '';
    for my $i (1..@{$ra_array->[$conllf{'ID'}]}-1){
	$s .= $ra_array->[0]->[$i];
	for my $j ($conllf{'PRED'}..@{$ra_array}-1){
	    $s .= "\t" . $ra_array->[$j]->[$i];
	}
	$s .= "\n";
    }
    return $s . "\n";
}

sub conllarray_to_columnstring($) {
    my $ra_array = shift;
    my $s = '';
    for my $i (1..@{$ra_array->[$conllf{'ID'}]}-1){
	$s .= $ra_array->[0]->[$i];
	for my $j (1..@{$ra_array}-1){
	    $s .= "\t" . $ra_array->[$j]->[$i];
	}
	$s .= "\n";
    }
    return $s . "\n";
}

sub conllarray_to_rowstring($) {
    my $ra_array = shift;
    my $s = '';
    my $f = 0;
    foreach my $ra (@{$ra_array}){
	$s .= "f$f\t"; $f++;
	$s .= join(" ", @{$ra}); # [1..@{$ra}-1]);
	$s .= "\n";
    }
    return $s . "\n";
}



sub itjhadjmat_to_conlladjmat {
    my ($ra_conll, $ra_syntadjmat, $ra_srladjmat) = @_;
    my (@conlladjmat, $dim);
    my $ambisrl = 0; my $unconnsrl = 0;
    $dim = @{$ra_conll->[$conllf{'ID'}]}-1;
    foreach my $i (0..$dim){
	foreach my $j (0..$dim){
	    if (defined($ra_srladjmat->[$i][$j]) && $ra_srladjmat->[$i][$j]){
		# my @asrl = map { $_; } @{$ra_srladjmat->[$i][$j]};
		# print STDERR "$i, $j ", join(",", @asrl), "\n";
		foreach my $l (@{$ra_srladjmat->[$i][$j]}){
		    my @asplit = split /\|a/, $l;
		    my $srl = shift @asplit;
		    if (@asplit){
			# print STDERR ">>> $i $j: ", join(",", @asplit), "\n";
			my @arevsplit = reverse @asplit;
			my @rindex = ($j); # my @cindex = ();
			while (defined(my $syntl = shift @arevsplit)){
			    my @cindex;
			    # print STDERR "! $syntl\n";
			    while (defined(my $r = shift @rindex)){
				# print STDERR "!!\t$r\n";
				foreach my $k (0..$dim){
				    if ($ra_syntadjmat->[$r][$k]){
					push @cindex, $k if ($ra_syntadjmat->[$r][$k]->[0] eq $syntl);
					# print STDERR "!!!\t\t$k $syntl\n" if ($ra_syntadjmat->[$r][$k]->[0] eq $syntl);
				    }
				}
			    }
			    @rindex = @cindex;
			}
			# print STDERR "RINDEX: ", join(", ", @rindex), "\n";
			if (@rindex){
			    push @{$conlladjmat[$i][$rindex[0]]}, $srl;
			    $ambisrl++ if ($#rindex);
			}else{
			    $unconnsrl++;
			}
		    }else{
			push @{$conlladjmat[$i][$j]}, $srl;
			# print STDERR "> $i $j: $srl\n";
		    }
		}
	    }
	}
    }
    return [\@conlladjmat, $ambisrl, $unconnsrl];
}

sub itjhrec_to_srladjmat { # stores an input ITJH record in the output 2-dim array
    my $rec = shift;
    my @rows = split /\n+/, $rec;
    my @columns;
    my $c = 0;
    my @srladjmat;
    map {push @{$columns[$c++]}, '-1';} (0..$conllf{'PRED'}); # '-1' is the dummy leftmost node prefixing each sentence
    for my $row (@rows){
	$c = 0;
	my @asplit = split /\s+/, $row;
	map {push @{$columns[$c++]}, $_;} (@asplit[0..$ivanf{'POS'}], 
					   $asplit[$ivanf{'SFORM'}], $asplit[$ivanf{'SLEMMA'}], $asplit[$ivanf{'POS'}], 
					   @asplit[$ivanf{'HEAD'}..$ivanf{'SENSE'}]);
	next if ($asplit[$ivanf{'SENSE'}] eq '_');
	# my @asrl = map { '_' } (0..@rows);
	if (my @asense = split /\~/, $asplit[$ivanf{'SENSE'}]){
	    $columns[$conllf{'PRED'}]->[$asplit[$ivanf{'ID'}]] = $asense[0];
	    # print STDERR join(" ", @asense), "\n";
	    push @{$srladjmat[$asplit[$ivanf{'ID'}]][$asplit[$ivanf{'ID'}]]}, @asense[1..$#asense] 
		if (defined($asense[1]));
	    # print STDERR join(",", @{$srladjmat[$asplit[$ivanf{'ID'}]][$asplit[$ivanf{'ID'}]]}), "\n";
	}
	my $k = $ivanf{'SENSE'};
	while (defined($asplit[$k+1]) && defined($asplit[$k+2])){
	    # print STDERR $asplit[$ivanf{'ID'}], " ", $asplit[$k+1], " ", $asplit[$k+2], "\n";
	    push @{$srladjmat[$asplit[$ivanf{'ID'}]][$asplit[$k+1]]}, $asplit[$k+2];
	    $k += 2;
	}
    }
    return [\@columns, \@srladjmat];
}


sub itjhinvproj_to_array {
    my ($ra_itjh, $ra_srladjmat) = @_;
    my @conll;
    foreach my $ra (@{$ra_itjh}[$conllf{'ID'}..$conllf{'PRED'}]){
	push @conll, $ra;
    }
    foreach my $i (0..@{$ra_itjh->[0]}-1){
	next if (($ra_itjh->[$conllf{'PRED'}]->[$i] eq '_') || ($ra_itjh->[$conllf{'PRED'}]->[$i] eq '-1'));
	# my $p = 0;
	my @asrlcol;
	foreach my $j (0..@{$ra_itjh->[0]}-1){
	    if ($ra_srladjmat->[$i][$j] && @{$ra_srladjmat->[$i][$j]}){
		# $p = 1;
		my @asrl = split /[*:]/, $ra_srladjmat->[$i][$j]->[0];
		# $srl =~ s/^([^\*\:]+)([\*\:])/$1/g;
		push @asrlcol, $asrl[0];
	    }else{
		push @asrlcol, '_';
	    }
	}
	push @conll, \@asrlcol; # if ($p);
    }
    return \@conll;
} 

sub conllrec_to_array($){ # stores an input CoNLL record in the output array
    my $rec = shift;
    my @rows = split /\n+/, $rec;
    my @columns;
    my $c = 0;
    map {push @{$columns[$c++]}, '-1';} split /\s+/, $rows[0]; # '-1' is the dummy leftmost node prefixing each sentence
    for my $row (@rows){
	$c = 0;
	map {push @{$columns[$c++]}, $_;} split /\s+/, $row;
    }
    return \@columns;
}

sub _lift {
    my ($arg, $ra_headcol, $ra_adjmat, $bank, $rh_lineargovlabel, $ra_conll, $ra_connv, $debug) = @_;

    my $sep = $bank ? ':' : "*"; 
    print "_lift $sep $arg->[0] $arg->[1] $arg->[2] $arg->[3] $arg->[4] $arg->[5]\n" if ($debug);
    my $ra_argancestors = 
	_ancestorspan($arg->[1], $arg->[0], $ra_headcol); # ->[0] is the argument, ->[1] is the predicate
    print "_lift $sep arg ancestor span: ", join(" ", @{$ra_argancestors}), "\n" if ($debug);
    my ($srl, $lemma);
    if ($bank){ # shift the non-projective (pred, arg)
	($srl, $lemma) = split /\:/, shift @{$ra_adjmat->[$arg->[1]]->[$arg->[0]]};
    }else{
	($srl, $lemma) = split /\*/, shift @{$ra_adjmat->[$arg->[1]]->[$arg->[0]]};
    }
    # $arg->[4] is the syntactic label licensing the predicate
    # $arg->[1] is the index of the predicate
    my $edgel = 
	$srl . '_' . $arg->[4] . $sep . $lemma . '-' . $arg->[1];
    # $rh_lineargovlabel->{$srl . '_' . $arg->[4]}++;
    print "*** $edgel\n" if ($debug);
    $ra_adjmat->[$arg->[1]]->[$arg->[0]] = 0 if (!@{$ra_adjmat->[$arg->[1]]->[$arg->[0]]}); # reset the (pred, arg) cell
    if (@{$ra_argancestors}){
	my $lineargov = pop @{$ra_argancestors};
	print "*** linearg $lineargov\n" if ($debug);
	$edgel = 
	    $srl . '_' . $arg->[4] . $sep . 
	    $ra_conll->[$conllf{'LEMMA'}]->[$lineargov] .'/'.$lemma . '-' . $arg->[1];
	if ($ra_adjmat->[$lineargov]->[$arg->[0]]){
	    push @{$ra_adjmat->[$lineargov]->[$arg->[0]]}, $edgel;
	}else{
	    $ra_adjmat->[$lineargov]->[$arg->[0]] = [$edgel];
	}
	--$ra_connv->[$arg->[1]];
	++$ra_connv->[$lineargov];
	my $pred = $ra_conll->[$conllf{'PRED'}]->[$lineargov] eq '_' ? 0 : 1;
	if ($pred){
	    $rh_lineargovlabel->{$ra_conll->[$conllf{'GPOS'}]->[$lineargov].$pred}++
	}else{
	    $rh_lineargovlabel->{$ra_conll->[$conllf{'GPOS'}]->[$lineargov].$pred.$ra_conll->[$conllf{'LEMMA'}]->[$lineargov]}++;
	}
	if (!$bank && !isivanprojedge($ra_adjmat, $ra_connv, $lineargov, $arg->[0])){
	    # rm srl link
	    print STDERR ">>> $arg->[1] $lineargov $arg->[0]\n";
	    $ra_adjmat->[$lineargov]->[$arg->[0]] = 0;
	    --$ra_connv->[$arg->[1]]; --$ra_connv->[$arg->[0]];
	    return 0;
	    
	}else{
	    return 1;
	}
    }else{ # lift to root, ie rm srl edge
	--$ra_connv->[$arg->[1]]; --$ra_connv->[$arg->[0]];
	return 0;
    }
}


sub intersection {
    my ($i, $sizei) = (0, scalar(keys %{$_[0]}));
    my ($j, $sizej);
    for my $j (1..$#_){ # smallest hash
	$sizej = scalar(keys %{$_[$j]});
	($i, $sizei) = ($j, $sizej) if ($sizej < $sizei);
    }
    my @a_intersectionkey = keys %{$_[$i]};
    my $rh_set;
    while ($rh_set = shift){
	@a_intersectionkey = grep(exists $rh_set->{$_}, @a_intersectionkey);
    }
    my %intersection;
    # hash slice, @hash{$x, $y} equivalent to ($hash{$x}, $hash{$y})
    @intersection{@a_intersectionkey} = (); # hash slice, undef assigned to each key, 
    # @intersection{@a_intersectionkey} = (1) x @a_intersectionkey;
    return \%intersection;
}

sub include { # include elements in $rh_b in $rh_a
    my ($rh_a, $rh_b) = @_;
    my ($k, $v);
    while (($k, $v) = each %$rh_b){
	$rh_a->{$k} = $v;
    }
}

sub union {
    my %union = ();
    while (@_){
	@union{keys %{$_[0]}} = (); # hash slice
	shift;
    }
    return \%union;
}

sub difference {
    my %difference;
    @difference{keys %{$_[0]}} = ();
    delete @difference{keys %{$_[1]}}; # hash slice, keys in %{$_[1]} deleted from %difference
    return \%difference;
}

sub symmetricdifference { # xor difference
    my %symdifference;
    while (defined(my $rh_set = shift(@_))){
	for my $elem (keys %{$rh_set}){
	    $symdifference{$elem}++;
	}
    }
    delete @symdifference{grep(($symdifference{$_}%2) == 0, keys %symdifference)}; # hash slice
    return \%symdifference;
}

sub comparison {
    my ($rh_seta, $rh_setb) = @_;
    my @intersectionk = grep(exists($rh_seta->{$_}), keys %{$rh_setb}); 
    return 'disjoint' unless (@intersectionk);
    return 'equal' if (@intersectionk == keys(%$rh_seta) and @intersectionk == keys(%$rh_setb));
    return 'psuperset' if (@intersectionk == keys(%$rh_setb));
    return 'psubset' if (@intersectionk == keys(%$rh_seta));
    return 'pintersect';    
}


END { }

1;
