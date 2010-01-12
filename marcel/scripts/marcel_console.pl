#!/usr/bin/perl -w
##################

use strict;

sub usage() {
	print "marcel_console.pl <prog1> [<prog2>] < <command_file>";
	exit;
}

my %cmd_set	= (
	enable	=> 1,
	disable	=> 2,
	enable_list	=> 3,
	disable_list	=> 4,
	enable_list_item	=> 5,
	disable_list_item	=> 6,
);

my $output_path	= '/tmp';
my $fifo_path	= '/tmp';

my $prog1	= shift;
unless (defined $prog1) {
	usage();
}
my $prog2	= shift;

my $fifo1_fh;
my $fifo2_fh;

my $user	= $ENV{'USER'};

my $output1	= "${output_path}/${user}_marcel_console_1.log";
my $fifo1	= "${fifo_path}/${user}_marcel_console_1.fifo";
unless (-p $fifo1) {
	system "mkfifo ${fifo1}"
		and die "mkfifo ${fifo1}: $!\n";
}
open ($fifo1_fh, "> $fifo1")
	 or die "open ${fifo1}: $!\n";
system "MARCEL_SUPERVISOR_FIFO=${fifo1} $prog1 > $output1 2>&1 &";

if (defined $prog2) {
	my $output2	= "${output_path}/${user}_marcel_console_2.log";
	my $fifo2	= "${fifo_path}/${user}_marcel_console_2.fifo";
	unless (-p $fifo2) {
		system "mkfifo ${fifo2}"
			and die "mkfifo ${fifo2}: $!\n";
	}
	open ($fifo2_fh, "> $fifo2")
		or die "open ${fifo2}: $!\n";
	system "MARCEL_SUPERVISOR_FIFO=${fifo1} $prog2 > $output2 2>&1 &";
}

while (<>) {
	chomp;

	my @args	= split / /;
	my $cmd	= shift @args;
	if ($cmd eq 'q') {
		# q: Quit
		print "Quit\n";
		last;
	} elsif ($cmd eq 't') {
		# t: Temporisation (arg = duration in seconds)
		print "Temporisation: $args[0] seconds\n";
		sleep $args[0];
	} elsif ($cmd eq 'e' or $cmd eq 'd') {
		# e: Enable (args = <1|2> <vpnum>)
		# d: Disable (args = <1|2> <vpnum>)
		my $cmd_name	= ($cmd eq 'e')?'enable':'disable';
		my $cmd_code	= $cmd_set{$cmd_name};
		my $prog_num	= shift @args;
		my $vpnum	= shift @args;
		my $data	= pack 'LL', ($cmd_code, $vpnum);
		my $fh;
		if ($prog_num == 1) {
			$fh	= $fifo1_fh;
		} elsif ($prog_num == 2) {
			$fh	= $fifo2_fh;
		} else {
			die "invalid prog_num ${prog_num}\n";
		}
		print "${cmd_name} VP ${vpnum} on prog ${prog_num}\n";
		syswrite $fh, $data or die "write to fifo: $!\n";
	} elsif ($cmd eq 'E' or $cmd eq 'D') {
		# E: Enable list (args = <1|2> <vpnum> [<vpnum>]*)
		# D: Disable list (args = <1|2> <vpnum> [<vpnum>]*)
		my $cmd_name	= ($cmd eq 'E')?'enable':'disable';
		my $cmd_code	= $cmd_set{"${cmd_name}_list"};
		my $sub_cmd_code	= $cmd_set{"${cmd_name}_list_item"};
		my $prog_num	= shift @args;
		my $nb_vps	= scalar (@args);
		my $fh;
		if ($prog_num == 1) {
			$fh	= $fifo1_fh;
		} elsif ($prog_num == 2) {
			$fh	= $fifo2_fh;
		} else {
			die "invalid prog_num ${prog_num}\n";
		}

		my $data	= pack 'LL', ($cmd_code, $nb_vps);
		print "${cmd_name} ${nb_vps} vp(s) on prog ${prog_num}\n";
		syswrite $fh, $data or die "write to fifo: $!\n";
		foreach my $vp (@args) {
			$data	= pack 'LL', ($sub_cmd_code, $vp);
			print ". ${cmd_name} vp ${vp}\n";
			syswrite $fh, $data or die "write to fifo: $!\n";
		}
	} else {
		die "unknown command: $cmd\n";
	}
}

