#!/opt/gnu/bin/perl

#
# $Id: check.pl,v 1.1 1998/10/07 13:18:59 krueger Exp $
#

##########################################################
###
###
###
##########################################################

##########################################################
###      G L O B A L  -  V a r i a b l e s   #############
##########################################################
$verbose = 0;
$result  = 0;
$my_name = `hostname`;
chop($my_name);
$tmp = "/tmp/CHECK.log";
$MMDFTAILOR="/opt/mmdf/mmdftailor";
$DELIVER="/opt/mmdf/lib/deliver";
$CHECKQUE="/opt/mmdf/lib/checkque";

##########################################################
###         M A I N    P a r t        ####################
##########################################################

if($#ARGV>=0) {
  if($ARGV[0] eq "-v") { $verbose = 1; }
}

umask(0000);
open(logf, "> " . $tmp);
if($verbose == 0) { select(logf); }
&get_channel_list();
$result |= &check_deliver(); print "\n";
$result |= &check_tmp();     print "\n";
$result |= &check_queue();   print "\n";

$subject = sprintf("\"CHECK on %s returned %d mistmatches.\"",  $my_name,
		   $result);
print "$subject\n";
close(logf);

#if($verbose == 0) {
#  exec("/opt/mmdf/lib/v6mail -s " . $subject . " mail-watch <" . $tmp);
#}


##########################################################
###   build list of channels    ##########################
##########################################################
sub get_channel_list {
   local($cmd) = "expand $MMDFTAILOR | grep \"^MCHN \" | awk '{print \$2}' | sed 's/,//g' |";
   open(fp, $cmd);
   while(<fp>) {
      chop;
      $chn_lst{$_}++;
      $channel{$_} = "nok";
   }
}

##########################################################
###   check if all deliver's are running   ###############
##########################################################
sub check_deliver {
  local(@line, $proc, $chan);
  local(@tim) = "-l10 ", "-s -l10 -t48 ";
  local(@Ok) = 0, 0, 0;
  local($deliver, $fault) = "/opt/mmdf/lib/deliver -b ", 0;

  print "checking if deliver's are running.\n";
  open(fp, "ps -fu mmdf | grep \"$DELIVER\" |");
  while(<fp>) {
    @line = split(' ');
    if(index($line[4], ":") >=0) {
      if(index($line[7], "deliver") < 0) { next; }
      $proc =  join(' ', $line[7], $line[8], $line[9], $line[10]);
      for($i=8; $i<=$#line; $i = $i+1) {
	if(substr($line[$i], 0, 2) eq "-c" ) { $chan = $line[$i]; }
      }
      $chan =~ s/^-c//g;
      if($channel{$chan} eq "nok") { $channel{$chan} = "ok"; }
    }
    else {
      if(index($line[8], "deliver") < 0) { next; }
      $proc = join(' ', $line[8], $line[9], $line[10], $line[11]);
      for($i=9; $i<=$#line; $i = $i+1) {
	if(substr($line[$i], 0, 2) eq "-c" ) { $chan = $line[$i]; }
      }
      $chan =~ s/^-c//g;
      if($channel{$chan} eq "nok") { $channel{$chan} = "ok"; }
    }
  }

  foreach $chan (keys(chn_lst)) {
    print "channel '$chan' is $channel{$chan}.\n";
    if( $channel{$chan} ne "ok") {
      print "Restart of deliver for '$chan' \n";
      if( $chan eq "smtp" ) { $cmd = $deliver . $tim[1] . "-c" . $chan . " &\n"; }
      else { $cmd = $deliver . $tim[0] . "-c" . $chan . " &\n"; }
      print $cmd;
      $fault++;
#      if( ($i=fork())==0) { 
#	exec($cmd);
#	exit;
#      }
#      else { waitpid($i, 0);}
    }
  }
  if($fault) { 
    printf "started %d delivers\n", $fault;
    return(1);
  }
  else {
    print "\tall deliver's are running.\n";
    return (0);
  }
}

##########################################################
###   check for existenz of the directory /tmp/mmdf  #####
##########################################################
sub check_tmp {
  local($tmp_e) = 0;

  print "checking if '/tmp/mmdf' exists:";
  if( -x "/tmp/mmdf" ) { $tmp_e = 1; }
  if($tmp_e) { 
    print "  OK.\n";
    return (0);
  }
  else 
    { print " must create the directory.\n";
      mkdir("/tmp/mmdf", 0777);
      return (2);
    }

}

##########################################################
###   just run the program 'checkque'    #################
##########################################################
sub check_queue {
  local(@line, @chn, @chn_msgs, @chn_bytes);
  local($msgs, $cnt) = 0, 0;

  print "checking the MMDF-queue.\n";
  open(fp, "$CHECKQUE -z | ");
  while(<fp>) {
    print $_;
    if(substr($_, 0, 1) eq "\n") { next; }
    @line = split(' ');
    if(index($_, "queued")>0) { # line with numbers of queued messages
      $msgs = $line[4];
    }
    if( (index($_, " msg  ")>0) || (index($_, " msgs  ")>0) ){
      # line for channel
      $chn[$cnt] = $line[6];
      $chn_msgs[$cnt] = $line[0];
      $chn_bytes[$cnt] = $line[2];
      $cnt = $cnt + 1;
    }
  }
  if($cnt) {
    for($i=0; $i<$cnt; $i = $i +1) {
      printf "%d msgs / %d kBytes in channel %s.\n", $chn_msgs[$i],
      $chn_bytes[$i], $chn[$i];
    }
    return(4);
  }
  return(0);
}

