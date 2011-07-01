#!/usr/bin/perl -w

my $GVDISPLAY = 0;
# my $GVDISPLAY = 1;
my $PNGOUT = 0;
my $JPGOUT = 0;

foreach my $arg (@ARGV) {
    $JPGOUT = 1 if($arg eq '-jpg');
    $PNGOUT = 1 if($arg eq '-png');
    $GVDISPLAY = 0 if($arg eq '-nodisplay');
}

my @words = ();     # words of the current sentence
my @dheads = ();    # dependency head-indices
my @dlabels = ();   # dependency labels
my @roles = ();     # for each index, a set of semantic arguments
		    # expressed as (index, label) pairs

# a running list of the indices of the predicates seen thus far
my @predicates = ();

# establish colors for semantic predicates
my @PREDCOLORS = ("#cc0000", "#00cc00", "#0000ff",
		  "#cc00cc", "#00ccff", "#cc9900");
my @PREDBGS = ("#ffcccc", "#ccffcc", "#ccccff",
	       "#ffccff", "#ddeeff", "#ffddcc");

# sentence index
my $INDEX = 0;

while(<STDIN>) {
    my @elts = split;
    last if ($INDEX == 1);
    if(scalar(@elts) == 0) {
	++$INDEX;
	&dotshow(); # arguments passed via globals
	@words = ();
	@dheads = ();
	@dlabels = ();
	@roles = ();
	@predicates = ();
	next;
    }


    # relevant columns:
    # 1   : word-index
    # 6   : split-form
    # 9   : dep head-index
    # 10  : dep label
    # 11  : if not "_", then identity of a predicate
    # 12+ : if not "_", then rel label for role w.r.t. (n-11)'th pred
    my $i = $elts[0];
    my $w = $elts[5];
    my $dh = $elts[8];
    my $dl = $elts[9];
    my $p = $elts[10];
    my @predlabels = @elts[11 .. $#elts];

    # keep track of the predicates seen thus far
    push(@predicates, $i) if($p ne "_");

    # maintain basic info
    $words[$i] = $w;
    $dheads[$i] = $dh;
    $dlabels[$i] = $dl;

    # predicate info
    my @predinfo = ();
    foreach $pindex (0 .. $#predlabels) {
	if($predlabels[$pindex] ne "_") {
	    push(@predinfo, [$pindex, $predlabels[$pindex]]);
	}
    }
    $roles[$i] = \@predinfo;
}





sub dotshow {
    my $fname = sprintf("sent-%04d", $INDEX);
    print STDERR "processing $fname...";
    # create temporary dotfile
    open(DOT, "> $fname.dot") or die "couldn't create dot file";
    print DOT <<END;
digraph "sentence $INDEX" {
    graph [rotate=90, rankdir=TB, ordering=out, ranksep=0.3, nodesep=0.75, size="8,10.5!", margin=0.25];
END

    # add a root node
    $words[0] = "*";

    # create a reverse mapping for the semantic predicates
    my @ispred = map {-1} (0 .. $#words);
    foreach my $i (0 .. $#predicates) {
	$ispred[$predicates[$i]] = $i;
    }

    # establish node identities
    foreach my $i (0 .. $#words) {
	if($ispred[$i] != -1) { # a semantic predicate node
	    my $c = $PREDCOLORS[$ispred[$i] % scalar(@PREDCOLORS)];
	    my $bc = $PREDBGS[$ispred[$i] % scalar(@PREDCOLORS)];
	    print DOT "    n$i [label=\"$words[$i]\", color=\"$c\", style=filled, fillcolor=\"$bc\"];\n";
	} else {
	    print DOT "    n$i [label=\"$words[$i]\"];\n";
	}
    }

    # first, gather all edges (dependency and role) and then sort them
    # by their mod-index.  printing the edges in this order allows dot
    # to produce a more pleasing graph (cf. ordering=out).  edges are
    # stored as tuples (head, mod, label, color)
    my @edges = ();

    # add dependency edges
    foreach my $i (1 .. $#words) {
	my $h = $dheads[$i];
	my $l = $dlabels[$i];
	push(@edges, [$h, $i, $l, "black"]);
    }

    # add semantic role edges
    foreach my $i (1 .. $#words) {
	my $rlist = $roles[$i];
	foreach my $pair (@$rlist) {
	    my $predindex = $pair->[0];
	    my $predlabel = $pair->[1];
	    my $predhead = $predicates[$predindex];
	    my $c = $PREDCOLORS[$predindex % scalar(@PREDCOLORS)];
	    push(@edges, [$predhead, $i, $predlabel, $c]);
	}
    }

    # sort edges by mod-index
    my @sorted = sort {$a->[1] <=> $b->[1]} @edges;

    # print sorted edges
    foreach my $e (@sorted) {
	my ($h, $m, $l, $c) = @$e;
	if($c eq "black") {  # syntactic dependency
	    print DOT "    n$h -> n$m [label=\"$l\", color=black, fontcolor=black, style=bold, constraint=true];\n";
	} else {             # semantic role
	    print DOT "    n$h -> n$m [label=\"$l\", color=\"$c\", fontcolor=\"$c\", style=solid, constraint=false];\n"
	}
    }

#     # add dependency edges
#     print DOT "    edge [style=bold, constraint=true]\n";
#     foreach my $i (1 .. $#words) {
# 	my $h = $dheads[$i];
# 	my $l = $dlabels[$i];
# 	print DOT "    n$h -> n$i [label=\"$l\"];\n";
#     }

#     # add semantic role edges
#     print DOT "    edge [style=solid, constraint=false]\n";
#     foreach my $i (1 .. $#words) {
# 	my $rlist = $roles[$i];
# 	foreach my $pair (@$rlist) {
# 	    my $predindex = $pair->[0];
# 	    my $predlabel = $pair->[1];
# 	    my $predhead = $predicates[$predindex];
# 	    print DOT "    n$predhead -> n$i [label=\"$predlabel\", color=red, fontcolor=red];\n"
# 	}
#     }

    # end graph statement
    print DOT "}\n";

    # execute dot
    print STDERR " dot";
    if(system("dot -Tps $fname.dot > $fname.ps") != 0) {
	print STDERR "dot failed on $fname!   skipping...\n";
	unlink("$fname.dot");
	unlink("$fname.ps");
	next;
    }

    # optionally convert PS to PNG
    if($PNGOUT) {
	print STDERR " png";
	system("gs -r125 -q -dNOPAUSE -dBATCH -dTextAlphaBits=4 -dGraphicsAlphaBits=4 -sDEVICE=png16m -sOutputFile=$fname.png $fname.ps") == 0
	    or die "PNG conversion (via ghostscript) failed!";
	system("convert -rotate 90 $fname.png $fname.png") == 0
	    or die "PNG rotation (via convert) failed!";
    }

    # optionally convert PS to JPG
    if($JPGOUT) {
	print STDERR " jpg";
	system("gs -r125 -q -dNOPAUSE -dBATCH -dTextAlphaBits=4 -dGraphicsAlphaBits=4 -sDEVICE=jpeg -sOutputFile=$fname.jpg $fname.ps") == 0
	    or die "JPG conversion (via ghostscript) failed!";
	system("convert -rotate 90 $fname.jpg $fname.jpg") == 0
	    or die "JPG rotation (via convert) failed!";
    }

    # optionally display via GV
    if($GVDISPLAY) {
	print STDERR " disp";
	system("gv -g +0+0 $fname.ps") == 0
	    or die "gv failed!";
    }

    print STDERR " done\n";
}
