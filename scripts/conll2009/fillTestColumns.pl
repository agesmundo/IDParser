#!/usr/bin/perl
# use <file_name> <pad_line>

open FH, $ARGV[0];
while(<FH>){
	if(/^\s*$/){print "\n"; next;}
	@F=split;
	$F[8] = 0;  $F[13]=$F[12]; 
	$F[10] = $ARGV[1];
	push @F, map {"_"}(1..100);
	print join("\t", @F), "\n";
}
