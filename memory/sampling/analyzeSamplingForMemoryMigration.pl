#!/usr/bin/perl

sub mean {
    my ($dataref) = @_;

    my $sum = 0;
    my $length = scalar(@$dataref);
    foreach $value (@$dataref) {
	$sum += $value;
    }
    return $sum / $length;
}

sub covar {
    my ($xdataref, $ydataref) = @_;

    my $prod = 0;
    my $length = scalar(@$xdataref);
    for(my $i=0 ; $i<$length ; $i++) {
        #print "f(@$xdataref[$i]) = @$ydataref[$i]\n";
        $prod += @$xdataref[$i] * @$ydataref[$i];
    }
    my $x_m = mean($xdataref);
    my $y_m = mean($ydataref);
    return $prod/$length - $x_m*$y_m;
}

sub var {
    my ($dataref) = @_;

    my $var = 0;
    my $mean = mean($dataref);
    my $length = scalar(@$dataref);
    foreach $value (@$dataref) {
	$var += ($value - $mean)**2;
    }
    return $var/$length;
}

sub linearRegression {
    my ($xdataref, $ydataref, $yregref, $yerrorref, $correctError) = @_;
    my $mean_x = mean($xdataref);
    my $mean_y = mean($ydataref);

    my $var_x = var($xdataref);
    my $var_y = var($ydataref);

    my $covar_xy = covar($xdataref, $ydataref);

    my $a = $covar_xy / $var_x;
    my $b = -$a * $mean_x + $mean_y;
    my $r = $covar_xy / sqrt($var_x) / sqrt($var_y); # pearson coefficient

    my $l = scalar(@$xdataref);
    for(my $i=0 ; $i<$l ; $i++) {
        my $reg=$a * @$xdataref[$i] + $b;
        push(@$yregref, $reg);
        my $error = ($reg /@$ydataref[$i] * 100) - 100;
        push(@$yerrorref, $error);

        #if ($error >= $correctError || $error <= -$correctError) {
        #print "Warning. The value for @$xdataref[$i] ($error) differs by more than $correctError% to the estimation \n";
        #}
    }

    return ($a, $b, $r);
}

sub fixFilenameAndHostname {
    my ($filename, $hostname) = @_;
    my $pathname = ".";

    if ($hostname eq "") {
        use Sys::Hostname;
        $hostname = hostname();
    }
    if ($filename eq "") {
        $pathname = $ENV{PM2_SAMPLING_DIR};
        if ($pathname eq "") {
            $pathname = "/var/local/pm2";
        }
        $pathname .= "/memory";
        $filename = "$pathname/sampling_for_memory_migration_$hostname.dat";
    }
    return ($pathname, $filename, $hostname);
}

sub filter {
    my ($xlistref, $ylistref, $xfilteredref, $yfilteredref, $x_min, $x_max) = @_;

    my $l = scalar(@$xlistref);
    for(my $i=0 ; $i<$l ; $i++) {
        if (@$xlistref[$i] >= $x_min && @$xlistref[$i] <= $x_max) {
            push(@$xfilteredref, @$xlistref[$i]);
            push(@$yfilteredref, @$ylistref[$i]);
        }
    }
}

sub help {
    print "Syntax: analyzeSampling.pl [<hostname>] [-file <sampling filename> -model <model filename>] \n";
    print "            [-min <minimum value for the x coordinates>] [-max <maximum value for the x coordinates>]\n";
    print "            [-source <node identifier>] [-dest <node identifier>]\n";
    print "            [-plot] [-dumb] [-jpeg]\n";
    print "  Performs a linear regression for the cost of the memory migration on hostname\n";
    print "  with:\n";
    print "      <hostname>   specifies the name of the machine for the sampling results (by default, uses the result of the command 'uname -m')\n";
    print "      -file        specifies the filename for the sampling results (by default get the result file of the specified machine from the PM2 directory)\n";
    print "      -plot        displays the plot for the sampling result (by default does not display the plot)\n";
    print "      -dumb        displays the plot in dumb mode (by default in graphical mode)\n";
    exit;
}

sub gnuplot {
    my ($outputfile, $terminal, $source, $dest, $plot) = @_;

    open gnuplot,$gnuplot=">${outputfile}.gnu" or die "Cannot open $gnuplot: $!";
    if ($terminal ne "") {
        print gnuplot "set terminal $terminal\n";
    }
    print gnuplot "set title \"Source $source - Dest $dest\"\n";
    print gnuplot "set xlabel \"Number of bytes\"\n";
    print gnuplot "set ylabel \"Migration time (nanosecondes)\"\n";
    if ($terminal eq "jpeg") {
        print gnuplot "set output \"${outputfile}_model.jpg\"\n";
    }
    print gnuplot "plot '${outputfile}.dat' using 1:2 title \"Original\" with lines, '${outputfile}.dat' using 1:3 title \"Regression\" with lines\n";

    if ($terminal ne "jpeg") {
        print gnuplot "pause -1\n";
    }

    print gnuplot "set ylabel \"Bandwidth (Mbytes/secondes)\"\n";
    if ($terminal eq "jpeg") {
        print gnuplot "set output \"${outputfile}_bandwidth.jpg\"\n";
    }
    print gnuplot "plot '${outputfile}.dat' using 1:4 title \"Original\" with lines, '${outputfile}.dat' using 1:5 title \"Regression\" with lines\n";

    if ($terminal ne "jpeg") {
        print gnuplot "pause -1\n";
    }

    print gnuplot "set ylabel \"Error (Original vs. Regression)\"\n";
    if ($terminal eq "jpeg") {
        print gnuplot "set output \"${outputfile}_error.jpg\"\n";
    }
    print gnuplot "plot '${outputfile}.dat' using 1:6 title \"Error\" with lines\n";

    if ($terminal ne "jpeg") {
        print gnuplot "pause -1\n";
    }

    close(gnuplot);
    if ($plot) {
        system("gnuplot $outputfile.gnu");
    }
}

# Main program
# Linear regression to exhibit a cost model y = a*x + b.
#      x = number of bytes
#      y = migration cost.
# xdatas and ydatas are matrices indiced on the node id. The cell
# [source][dest] of the x (resp. y) matrix will contain all the x
# (resp. y) values for the sampling done between the nodes source and
# dest.

# Set the default parameters
my $plot = 0;
my $terminal = "";
my $filename = "";
my $modelfilename = "";
my $hostname = "";
my $pathname = "";
my $x_min = 0;               # Minimum size for the x coordinates
my $x_max = -1;              # Maximum size for the x coordinates
my $source_min=0;
my $source_max=10;
my $dest_min=0;
my $dest_max=10;

my $correctError = 10;

# Read the command-line parameters
for(my $i=0 ; $i<scalar(@ARGV) ; $i++) {
    if ($ARGV[$i] eq "-help") {
        help();
    }
    elsif ($ARGV[$i] eq "-min") {
	$x_min=$ARGV[$i+1];
	$i++;
    }
    elsif ($ARGV[$i] eq "-max") {
	$x_max=$ARGV[$i+1];
	$i++;
    }
    elsif ($ARGV[$i] eq "-source") {
	$source_min=$source_max=$ARGV[$i+1];
	$i++;
    }
    elsif ($ARGV[$i] eq "-dest") {
	$dest_min=$dest_max=$ARGV[$i+1];
	$i++;
    }
    elsif ($ARGV[$i] eq "-plot") {
	$plot = 1;
    }
    elsif ($ARGV[$i] eq "-dumb") {
	$terminal = "dumb";
    }
    elsif ($ARGV[$i] eq "-jpeg") {
	$terminal = "jpeg";
    }
    elsif ($ARGV[$i] eq "-file") {
	$filename = $ARGV[$i+1];
        $i++;
    }
    elsif ($ARGV[$i] eq "-model") {
	$modelfilename = $ARGV[$i+1];
        $i++;
    }
    else {
        $hostname = $ARGV[$i];
    }
}

# Get the location of the sampling results output file
($pathname, $filename, $hostname) = fixFilenameAndHostname($filename, $hostname);

# Initialise the datas to store the x and y values.
my @xdatas;
my @ydatas;
for $source ($source_min .. $source_max) {
    for $dest ($dest_min .. $dest_max) {
	my @xlist = ();
	$xdatas[$source][$dest] = \@xlist;
	my @ylist = ();
	$ydatas[$source][$dest] = \@ylist;
    }
}
for $source ($source_min .. $source_max) {
    for $dest ($dest_min .. $dest_max) {
	my $xlistref = $xdatas[$source][$dest];
	my $ylistref = $ydatas[$source][$dest];
	#print "Lists $xlistref $ylistref\n";
    }
}

# Read the x and y values and store them in the appropriate list
open input,$input="$filename" or die "Cannot open $input: $!";
my $head = <input>; # read out the first line
while(<input>) {
    chomp;
    ($source, $dest, $pages, $size, $cost) = split();
    my $xlistref = $xdatas[$source][$dest];
    my $ylistref = $ydatas[$source][$dest];
    push(@$xlistref, $size);
    push(@$ylistref, $cost);
}
close(input);

if ($modelfilename eq "") {
    $modelfilename = "$pathname/model_for_memory_migration_$hostname.dat";
}
open result,$result=">$modelfilename" or die "Cannot open $result: $!";
print "Source\tDest\tX_min\tX_max\tSlope\tIntercept\tCorrelation\tBandwidth(MB/s)\n";
print result "Source\tDest\tX_min\tX_max\tSlope\tIntercept\tCorrelation\tBandwidth(MB/s)\n";

# For each pair $source $dest, perform the linear regression and plot
# the result
# The interval [$x_min .. $x_max] is going to be split in
# sub-intervals in which the linear regression gives good results.
for $source ($source_min .. $source_max) {
    for $dest ($dest_min .. $dest_max) {
	my $xlistref = $xdatas[$source][$dest];
	my $ylistref = $ydatas[$source][$dest];
	next if (scalar(@$xlistref) == 0);

        # Filter the values
        my @xfiltered = ();
        my @yfiltered = ();
        my @yreg = ();
        my @yerror = ();
        my $a, $b, $r;

        if ($x_max == -1) {
            $x_max = @$xlistref[scalar(@$xlistref)-1];
        }

        my $globaloutputfile = "$pathname/sampling_${source}_${dest}";
        open globaloutput,$globaloutput=">${globaloutputfile}.dat" or die "Cannot open $globaloutput: $!";
	print globaloutput "Bytes\tMigration_time_(nanosec)\tRegression_(nanosec)\tMeasured_Bandwidth_(MB/s)\tRegression_Bandwidth_(MB/s)\tError\n";

        my $intervals = 0;
        my $x_current_min = $x_min;
        my $x_current_max = $x_max;

        do {
            my $done = 1;
            # We look for the biggest interval [$x_current_min .. $X] with
            # $X <= $x_current_max for which the correlation is good.
            do {
              DO_NEXT: {
                  $done = 1;
                  @xfiltered = ();
                  @yfiltered = ();
                  @yreg = ();
                  @yerror = ();

                  #print "performs regression for $x_current_min .. $x_current_max\n";

                  # filters data
                  filter($xlistref, $ylistref, \@xfiltered, \@yfiltered, $x_current_min, $x_current_max);
                  next DO_NEXT if (scalar(@xfiltered) == 0) || (scalar(@xfiltered) == 1);

                  # Performs the linear regression
                  ($a, $b, $r) = linearRegression(\@xfiltered, \@yfiltered, \@yreg, \@yerror, $correctError);

                  if ((@yreg[0] < 0) || ($r < 0.992)) {
                      $done = 0;
                      $x_current_max = @xfiltered[scalar(@xfilerered)-2];
                  }
              }
            } while ($done == 0);


            $intervals += 1;
	    my $l = scalar(@xfiltered) - 1;
            if ( $l == 0 ) {
                push(@yreg, @yfiltered[0]);
            }
            my $bandwidth = @xfiltered[$l] / @yreg[$l] * 1000000000 / 1024 / 1024;
            printf result "$source\t$dest\t$x_current_min\t$x_current_max\t%8.5f\t%8.5f\t%8.5f\t%8.5f\n", $a, $b, $r, $bandwidth;
            printf "$source\t$dest\t$x_current_min\t$x_current_max\t%8.5f\t%8.5f\t%8.5f\t%8.5f\n", $a, $b, $r, $bandwidth;

            my $outputfile = "$pathname/sampling_${source}_${dest}_${x_current_min}_${x_current_max}";
            open output,$output=">${outputfile}.dat" or die "Cannot open $output: $!";
	    print output "Bytes\tMigration_time_(nanosec)\tRegression_(nanosec)\tMeasured_Bandwidth_(MB/s)\tRegression_Bandwidth_(MB/s)\tError\n";
            my $l = scalar(@xfiltered);
            for(my $i=0 ; $i<$l ; $i++) {
                my $bandwidth = @xfiltered[$i] / @yfiltered[$i] * 1000000000 / 1024 / 1024;
		my $bandwidth2 = @xfiltered[$i] / @yreg[$i] * 1000000000 / 1024 / 1024;
                print output "@xfiltered[$i]\t@yfiltered[$i]\t@yreg[$i]\t$bandwidth\t$bandwidth2\t@yerror[$i]\n";
                print globaloutput "@xfiltered[$i]\t@yfiltered[$i]\t@yreg[$i]\t$bandwidth\t$bandwidth2\t@yerror[$i]\n";
            }
            close(output);

            gnuplot($outputfile, $terminal, $source, $dest, $plot);

            $x_current_min = $x_current_max+1;
            $x_current_max = $x_max;
        } while ($x_current_min < $x_max);

        close(globaloutput);
        if ($intervals != 1) {
            gnuplot($globaloutputfile, $terminal, $source, $dest, $plot);
        }
    }
}

close(result);

print "Model saved in <$modelfilename>\n";
