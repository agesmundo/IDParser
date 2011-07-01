#!/usr/bin/perl
use warnings;
use strict;
use CoNLL;
use Carp;

$/ = "";
my $nrec = 0;
my $projrec = 0;

while (<>){
    $nrec++;
    chomp;
    if (0){ #(0){
	my $ra_conllrec = conllrec_to_array($_);
	my $ra_srladjmats = srladjmats($ra_conllrec);
	next if (!isivanproj($ra_srladjmats->[0], $ra_srladjmats->[3]));
	my $ra_syntadjmat = syntadjmat($ra_conllrec);
	my $ra_syntclosure = closure($ra_syntadjmat->[0], $ra_syntadjmat->[1]);
	next if (!ismarcusproj($ra_syntadjmat->[0], $ra_syntclosure, $ra_syntadjmat->[1]));
	$projrec++;
	# print conllarray_to_columnstring($ra_conllrec);
        print $_, "\n\n";
	last if ($projrec > 3000);
	next;
    }

    my $ra_conllrec = conllrec_to_array($_);
    print conllarray_to_rowstring($ra_conllrec);
    print conllarray_to_columnstring($ra_conllrec);
    
    my $ra_syntadjmat = syntadjmat($ra_conllrec);
    print "synt adjmat\n", adjmat_to_string($ra_syntadjmat->[0]);
    my $ra_srladjmats = srladjmats($ra_conllrec);
    print "srl adjmat\n", adjmat_to_string($ra_srladjmats->[0]);
    print "propb adjmat\n", adjmat_to_string($ra_srladjmats->[1]);
    print "nomb adjmat\n", adjmat_to_string($ra_srladjmats->[2]);
    my $ra_syntclosure = closure($ra_syntadjmat->[0], $ra_syntadjmat->[1]);
    print "synt closure\n", closure_to_string($ra_syntclosure, $ra_syntadjmat->[1]);
    my $ra_srlclosure = closure($ra_srladjmats->[0], $ra_srladjmats->[3]);
    print "srl closure\n", closure_to_string($ra_srlclosure, $ra_srladjmats->[3]);
    my $ra_syntprojmat = marcusprojmat($ra_syntadjmat->[0], $ra_syntclosure, $ra_syntadjmat->[1]);
    print "synt projmat\n", projmat_to_string($ra_syntprojmat->[0]);
    my $ra_srlprojmat = ivanprojmat($ra_srladjmats->[0], $ra_srladjmats->[3]);
    print "srl projmat\n", projmat_to_string($ra_srlprojmat->[0]);
    print "P\n" if (ismarcusproj($ra_syntadjmat->[0], $ra_syntclosure, $ra_syntadjmat->[1]));
    print "NP\n" if (!isivanproj($ra_srladjmats->[0], $ra_srladjmats->[3]));
    
    last;
}

print STDERR "$nrec CoNLL records read, $projrec CoNLL projective records sampled\n\n";

