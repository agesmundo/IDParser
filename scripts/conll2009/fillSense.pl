#!/usr/bin/perl
# use <file_name> 

@sense_suffix = {}
open FH, $ARGV[0];
while(<FH>){
	if(/^\s*$/){
		print "\n"; 
		next;
	}
	@F=split;
	$F[8] = 0;  
	$F[13]="_"; 
	$F[14]="_"; 
	$F[10] = $ARGV[1];
	print join("\t", @F), "\n";
}
