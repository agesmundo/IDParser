#!/usr/bin/perl
use warnings;
use strict;
use CoNLL;
use Carp;

my $rec = shift;
$/ = "";
my $nrec = 0;

while (<>){
    # next if (rand() > 0.1);
    $nrec++;
    next if ($nrec != $rec);
    chomp;
    my $ra_conll = conllrec_to_array($_);
    my $ra_syntadjmat  = syntadjmat($ra_conll);
    my $ra_srladjmats  = srladjmats($ra_conll);
    print STDERR conllarray_to_rowstring($ra_conll);
    # print adjmat_to_string($ra_syntadjmat->[0]);
    # print adjmat_to_string($ra_srladjmats->[0]);
    my $dotcode = dot($nrec, $ra_conll, $ra_syntadjmat->[0], $ra_srladjmats->[0]);
    open DOT, "> conllrec$nrec.dot" or die "couldnt write to file conllrec$nrec.dot: $!";
    print DOT $dotcode;
    print STDERR "dot -Tps conllrec$nrec.dot > conllrec$nrec.ps\n";
    if (system("dot -Tps conllrec$nrec.dot > conllrec$nrec.ps")){
	print STDERR "dot -Tps conllrec$nrec.dot > conllrec$nrec.ps FAILED\n";
	unlink("conllrec$nrec.dot"); unlink("conllrec$nrec.ps");
	exit;
    }
    print STDERR "gv conllrec$nrec.ps\n";
    if (system("gv conllrec$nrec.ps")){
	print STDERR "gv conllrec$nrec.ps FAILED\n";
    }
    last;
}
