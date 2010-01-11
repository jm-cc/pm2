#!/usr/bin/perl -w
##################

my %cmd_set	= (
	enable	=> 1,
	disable	=> 2,
	enable_list	=> 3,
	disable_list	=> 4,
	enable_list_item	=> 5,
	disable_list_item	=> 6,
);

my $prog1	= shift;
my $prog2	= shift;

my $output_path	= '/tmp';
my $output1	= "${output_path}/$prog1.log";
my $output2	= "${output_path}/$prog2.log";

my $fifo_path	= '/tmp';
my $fifo1	= "${fifo_path}/$prog1.fifo";
my $fifo2	= "${fifo_path}/$prog2.fifo";
my $fifo1_fh;
my $fifo2_fh;

unless (-p $fifo_1) {
	system "mkfifo ${fifo1}"
		and die "mkfifo ${fifo1}: $!\n";
}

unless (-p $fifo_2) {
	system "mkfifo ${fifo2}"
		and die "mkfifo ${fifo2}: $!\n";
}

open ($fifo1_fh, ">> $fifo_1") or die "open ${fifo1}: $!\n";
open ($fifo2_fh, ">> $fifo_2") or die "open ${fifo2}: $!\n";

system "MARCEL_SUPERVISOR_FIFO=${fifo1} $prog1 > $output1 2>&1 &";
system "MARCEL_SUPERVISOR_FIFO=${fifo1} $prog2 > $output2 2>&1 &";

while (<>) {
	chomp;

	my @args	= split / /;
	my $cmd	= shift @args;
	if ($cmd eq 'q') {
		# q: Quit
		last;
	} elsif ($cmd eq 't') {
		# t: Temporisation (arg = duration in seconds)
		sleep $arg;
	} elsif ($cmd eq 'e' or $cmd eq 'd') {
		# e: Enable (args = <1|2> <vpnum>)
		# d: Disable (args = <1|2> <vpnum>)
		my $cmd_code	= ($cmd eq 'e')?$cmd_set{'enable'}:$cmd_set{'disable'};
		my $prog_num	= shift @args;
		my $vpnum	= shift @args;
		my $data	= pack 'LL', ($cmd_code, $vpnum);
		my $fh;
		if ($prog_num == 1) {
			$fh	= $fifo1_fh;
		elsif ($prog_num == 2) {
			$fh	= $fifo2_fh;
		} else {
			die "invalid prog_num ${prog_num}\n";
		}
		write $fh, $data or die "write to fifo: $!\n";
	} elsif ($cmd eq 'E' or $cmd eq 'D') {
		# E: Enable list (args = <1|2> <vpnum> [<vpnum>]*)
		# D: Disable list (args = <1|2> <vpnum> [<vpnum>]*)
		my $cmd_code	= ($cmd eq 'E')?$cmd_set{'enable_list'}:$cmd_set{'disable_disable_list'};
		my $sub_cmd_code	= ($cmd eq 'E')?$cmd_set{'enable_list_item'}:$cmd_set{'disable_disable_list_item'};
		my $prog_num	= shift @args;
		my $nb_vps	= scalar ($@args);
		my $fh;
		if ($prog_num == 1) {
			$fh	= $fifo1_fh;
		elsif ($prog_num == 2) {
			$fh	= $fifo2_fh;
		} else {
			die "invalid prog_num ${prog_num}\n";
		}

		my $data	= pack 'LL', ($cmd_code, $nb_vps);
		write $fh, $data or die "write to fifo: $!\n";
		foreach my $vp (@args) {
			$data	= pack 'LL', ($sub_cmd_code, $vp);
			write $fh, $data or die "write to fifo: $!\n";
		}
	} else {
		die "unknown command: $cmd\n";
	}
}

