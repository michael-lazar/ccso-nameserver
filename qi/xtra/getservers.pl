#!/usr/bin/perl

# getservers.pl, by Martin Hamilton - martin@mrrl.lut.ac.uk

$SITE="site:University of Illinois at Urbana-Champaign";
$SERVER="server:ns.uiuc.edu";
#$MYDOMAIN="uiuc.edu";

$QI="/usr/local/libexec/qi";
$PH="/usr/local/bin/ph";

$A=1;

# Use these lines the first time this script is run to add the entries.
#$L="$QI <<EOF\nadd type=\"serverlist\" alias=\"nssrvr-";
#$R="\" name=\"ns-servers - Ph NS Servers\" email=\"Info.Server@ns.uiuc.edu\" text=\"";

# Use these lines to update the entries once a week or so from cron
$L="$QI <<EOF\nchange alias=\"nssrvr-";
$R="\" make text=\"";

open(SERVERS, "$PH -l -s ns.nwu.edu ns-servers return text|")
  || die "Problem opening pipe to $PH: $!";

$junk = <SERVERS>;

# might want to add $MYDOMAIN here...
$results = "${L}${A}${R}$SITE\n$SERVER\n";

while(<SERVERS>){
  $results .= $_, next unless /^----------------------------------------$/;

  $A++;
  $results .= "\"\nquit\nEOF\n\n";
  $results .= "${L}${A}${R}" unless eof(SERVERS);
}

close(SERVERS);

$* = 1; $results =~ s/\n"\n/"\n/g;

print $results;
