
# perl library for qi queries
#if ($#ARGV < 0) {die "Usage: really {name|id|login@host}...\n";}

require 'open2.pl';
require 'sys/socket.ph';

#$LongHost = `hostname`; chop($LongHost);
#$ShortHost = $LongHost; $ShortHost =~ s/\..*//;
#$QiHost = 'ns.uiuc.edu';
#$QiPort = 'ns';
#
#&QiConnect(S);
#&QiLogin;

#  example code
#  $name = &GetQiField($q,"name");

sub QiConnect
{
  ($s) = @_;
  $sockaddr = 'S n a4 x8';
  ($name,$aliases,$proto) = getprotobyname('tcp');
  ($name,$aliases,$QiPort) = getservbyname($QiPort,'tcp')
    unless $QiPort =~ /^\d+$/;
  ($name,$aliases,$type,$len,@thisaddr) = gethostbyname($LongHost);
  ($name,$aliases,$type,$len,@thataddr) = gethostbyname($QiHost);
  $that = pack($sockaddr,&AF_INET,$QiPort,$thataddr[0]);
  for($fromport=1023;$fromport;$fromport--)
  {
    $this = pack($sockaddr,&AF_INET,$fromport,$thisaddr[0]);
    socket($s,&AF_INET,&SOCK_STREAM,$proto) || die "Qi socket: $!\n";
    if (bind($s,$this)) {last;}
  }
  if (!$fromport) {die "Ran out of ports.\n";}

  connect($s,$that) || die "Qi connect: $!";

  select($s); $|=1; select(STDOUT);
}

sub QiLogin
{
  ($code,$resp) = &QiCommand("login umng-$ShortHost");
  if ($code != 301) {die "Qi login failed $code.";}
  ($code,$resp) = &QiCommand("email umng");
  if ($code != 200) {die "Qi login failed $code.";}
}

sub QiCommand
{
  $cmd = $_[0];
  print S "$cmd\n";
  $resp = '';
  $code = 500;
  while (<S>)
  {
    $resp .= $_;
    chop;
    $code=(split(':'))[0];
    if ($code >= 200) {return($code,$resp);}
  }
  die "Lost qi connection.\n";
}

sub GetQiField
{
  ($query,$field) = @_;
  ($code,$resp) = &QiCommand("query $query return $field");
  $val = '';
  foreach $l (split('\n',$resp))
  {
    if ($l =~ /^-200:1:/)
    {
      $l =~ s/-200:1:[^:]*: //;
      $val .= "$l\n";
    }
    elsif ($l =~ /^-200:2:/)
    {
      print STDERR "More than one ph entry for $query.\n";
      last;
    }
  }
  chop($val);
  $val;
}
