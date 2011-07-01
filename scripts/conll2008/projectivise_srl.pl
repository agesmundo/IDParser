#!/usr/bin/perl
use warnings;
use strict;
use CoNLL;
use Carp;

my $conllfile = shift;
open CONLL, "< $conllfile" or die "couldnt open file $conllfile: $!\n";
my $closurefile = shift;
open CLOSURE, "< $closurefile" or die "couldnt open file $closurefile: $!\n";
my $rec = shift;
my $debug = 0;
$debug = shift;

$/ = "";
my $nrec = 0;
my $liftednb = 0; my $rmnb = 0;
my $liftedpb = 0; my $rmpb = 0;
my $srlnb = 0; my $srlpb = 0;
my $nb = 0; my $pb = 0;
my (%lineargovlabel, %linearsrlabel);
my ($nnblifted0, $nnblifted1, $nnbrm0, $nnbrm1, $npblifted0, $npblifted1, $npbrm0, $npbrm1);
my ($nnblifted, $nnbrm, $npblifted, $npbrm, $nbnproj, $pbnproj);

while ((defined(my $conllrec = <CONLL>)) && (defined(my $closurerec = <CLOSURE>))){
    $nrec++;
    if ($rec) {
	last if ($nrec > $rec);
	next if ($nrec != $rec);
    }
    # print STDERR $_;
    # chomp $_;
    print STDERR $conllrec if ($debug);
    chomp $conllrec; 
    chomp $closurerec;
    my $ra_conll = conllrec_to_array($conllrec);
    # print STDERR conllarray_to_columnstring($ra_conll);
    my $ra_syntadjmat = syntadjmat($ra_conll);
    
    my $ra_closure = closurerec_to_array($closurerec);
    print closure_to_string($ra_closure) if ($debug);
    # my $ra_closure = closure($ra_syntadjmat->[0]);
    # print STDERR closure_to_string($ra_closure);
    
    print STDERR adjmat_to_string($ra_syntadjmat->[0]) if ($debug);
    my $ra_srladjmats = srl_adjmats($ra_conll);
    $nb += @{$ra_srladjmats->[8]};
    $pb += @{$ra_srladjmats->[7]};
    # my $ra_srladjmats_1 = srl_adjmats($ra_conll);
    _srlprojstruct($ra_syntadjmat, $ra_closure, $ra_srladjmats);
    

    print STDERR "\n" if ($debug);
    print STDERR adjmat_to_string($ra_srladjmats->[0]) if ($debug);
    my $ra = projectivise_srladjmat($ra_srladjmats, $debug);
    $nnblifted0 += $ra->[0]->[0];
    $nnbrm0 += $ra->[0]->[1];
    $npblifted0 += $ra->[0]->[2];
    $npbrm0 += $ra->[0]->[3];
    $nnblifted1 += $ra->[1]->[0];
    $nnbrm1 += $ra->[1]->[1];
    $npblifted1 += $ra->[1]->[2];
    $npbrm1 += $ra->[1]->[3];
    
    $nbnproj += $ra->[2];
    $pbnproj += $ra->[3];

    # $nnblifted += $ra->[2]->[0];
    # $nnbrm += $ra->[2]->[1];
    # $npblifted += $ra->[2]->[2];
    # $npbrm += $ra->[2]->[3];

    print STDERR adjmat_to_string($ra_srladjmats->[0]) if ($debug);
    print STDERR "\n" if ($debug);
    
    # np_projectivise_srladjmat($ra_srladjmats_1, $debug);
    # print STDERR adjmat_to_string($ra_srladjmats_1->[0]);
    # print STDERR adjmat_to_string($ra_srladjmats->[1]); # PropBank
    # print STDERR adjmat_to_string($ra_srladjmats->[2]); # NomBank
    # print STDERR "in: ", join(" ", @{$ra_srladjmats->[3]}), "\n\n";
    # print STDERR "out: ", join(" ", @{$ra_srladjmats->[4]}), "\n\n";
    # print STDERR rara_to_string($ra_srladjmats->[5]); # PropBank predicates
    # print STDERR rara_to_string($ra_srladjmats->[6]); # NomBank predicates
    # print STDERR rara_to_string($ra_srladjmats->[7]); # PropBank srl
    # print STDERR rara_to_string($ra_srladjmats->[8]); # NomBank srl
    
    

}

print STDERR "$nrec CoNLL rec done\n\n";
printf STDERR ("H0: %d NomBank lifted %3.2f\n", $nnblifted0, ($nnblifted0*100)/$nb);
printf STDERR ("H0: %d NomBank removed %3.2f\n", $nnbrm0, ($nnbrm0*100)/$nb);
printf STDERR ("H0: %d PropBank lifted %3.2f\n", $npblifted0, ($npblifted0*100)/$pb);
printf STDERR ("H0: %d PropBank removed %3.2f\n", $npbrm0, ($npbrm0*100)/$pb);
printf STDERR ("H0: removed %3.2f\n", (($npbrm0+$nnbrm0)*100)/($nb+$pb));
printf STDERR ("H0: lifted %3.2f\n\n", (($npblifted0+$nnblifted0)*100)/($nb+$pb));

printf STDERR ("H1: %d NomBank lifted %3.2f\n", $nnblifted1, ($nnblifted1*100)/$nb);
printf STDERR ("H1: %d NomBank removed %3.2f\n", $nnbrm1, ($nnbrm1*100)/$nb);
printf STDERR ("H1: %d PropBank lifted %3.2f\n", $npblifted1, ($npblifted1*100)/$pb);
printf STDERR ("H1: %d PropBank removed %3.2f\n", $npbrm1, ($npbrm1*100)/$pb);
printf STDERR ("H1: removed %3.2f\n", (($npbrm1+$nnbrm1)*100)/($nb+$pb));
printf STDERR ("H1: lifted %3.2f\n\n", (($npblifted1+$nnblifted1)*100)/($nb+$pb));

printf STDERR "%d NomBank srls %d non-proj %3.2f\n", $nb, $nbnproj, ($nbnproj*100)/$nb;
printf STDERR "%d PropBank srls %d non-proj %3.2f\n", $pb, $pbnproj, ($pbnproj*100)/$pb;
printf STDERR "%d srls %d non-proj %3.2f\n", $pb+$nb, $pbnproj+$nbnproj, (($pbnproj+$nbnproj)*100)/($pb+$nb);

