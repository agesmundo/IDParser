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
    my $ra_ivan = conllarray_to_ivaneaclarray($ra_conll, $RHPB1, $RHNB1);
    print conllarray_to_columnstring($ra_ivan);
    # last if ($nrec > 68);
}

print STDERR "$nrec CoNLL records read\n";

