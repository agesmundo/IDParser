#!/usr/bin/perl

use strict; use warnings;
use Data::Dumper qw(Dumper);

my $infile = shift;
open IN_CONLL, "< $infile" or die "couldnt open file $infile: $!\n";
my $outfile = shift;
open OUT_DUMP, "> $outfile" or die "couldnt open file $outfile: $!\n";
my $nrow = 0;
my (%nb, %pb);
while (<IN_CONLL>){
    next if (/^\s*$/);
    chomp;
    my @fields = split /\s+/;
    if ($fields[10] ne '_'){ # PRED field
	if ($fields[3] =~ /^NN/){ # gold PoS field
	    $nb{$fields[6]}->{$fields[10]}++; # LEMMA field
	}else{
	    $pb{$fields[6]}->{$fields[10]}++;
	}
    }
    $nrow++;
}

$Data::Dumper::Indent = 1;
$Data::Dumper::Varname = 'RHPB';
print OUT_DUMP Dumper(\%pb);
$Data::Dumper::Varname = 'RHNB';
print OUT_DUMP Dumper(\%nb );

print STDERR "$nrow CoNLL rows read\n";
