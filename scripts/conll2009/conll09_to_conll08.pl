#!/usr/bin/perl
# param:
# <CoNLL 2009 format>
# <OutFile : CoNLL 2008 format>

$inFile = $ARGV[0];
$outFile = $ARGV[1];

open(IN, "$inFile");
open(OUT, ">$outFile");
@lines = <IN>;
close(IN);
$lastLineWasEmpty = 1;
foreach $line  (@lines){
	$_ = $line;
	if(/(\S+)/){
		$lastLineWasEmpty = 0;
		$idCol = "$1";
		$_ = $';
		$colCount = 1;
		while(/(\S+)/){
			$colCount++;
			if($colCount == 2 ){
				$formCol ="$1";	
				$sFormCol ="$1";
			}
			if($colCount == 3 ){
				$lemmaCol ="$1";	
				$sLemmaCol ="$1";	
			}
			if($colCount == 5 ){
				$gPosCol ="$1";	
			}
			if($colCount == 6 ){
				$pPosCol ="$1";	
				$pPosSCol ="$1";	
			}
			if($colCount == 9 ){
				$headCol ="$1";	
			}
			if($colCount == 11 ){
				$depRelCol ="$1";	
			}
			if($colCount == 14 ){
				$predCol ="$1";	
			}
			if($colCount >14  ){
				$argCols .="\t$1";	
			}
			$_ = $';
		}
		$outLine ="$idCol\t$formCol\t$lemmaCol\t$gPosCol\t$pPosCol\t$sFormCol\t$sLemmaCol\t$pPosSCol\t$headCol\t$depRelCol\t$predCol$argCols\n";
		$argCols="";
		print OUT $outLine;
		
		#check the total number of colums found 
		if($colCount<14){
			print "Error: wrong number of column in the input file, at line: $line\n"
		}

	}
	else{

		#check for consecutive empty lines
		if($lastLineWasEmpty){
			print "Warning: consecutive empty lines in input file";
		}

		$lastLineWasEmpty = 1;
		print OUT "\n";
	}
}
