#!/usr/bin/perl
use warnings;
use strict;
use CoNLL;
use Carp;

my $conllfile = shift;
my $dumpfile = shift;
open CONLL, "< $conllfile" or die "couldnt open file $conllfile: $!\n";
open DUMP, "< $dumpfile" or die "couldnt open file $dumpfile: $!\n";
my ($RHPB1, $RHNB1);

{
    local $/ = undef;
    eval(<DUMP>);
}


$/ = "";
my $nrec = 0;
while (<CONLL>){
    $nrec++;
    chomp;
    my $ra_conll = conllrec_to_array($_);
    # print conllarray_to_rowstring($ra_conll);
    # print conllarray_to_columnstring($ra_conll);
    my $ra_ivan = conllarray_to_ivanarray($ra_conll, $RHPB1, $RHNB1);
    print conllarray_to_columnstring($ra_ivan);
}

print STDERR "$nrec CoNLL records read\n";


