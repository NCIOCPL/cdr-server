#----------------------------------------------------------------------
#
# $Id: CdrCmd.pl,v 1.1 2001-11-28 19:31:03 bkline Exp $
#
# Test client (Perl version) for sending commands to CDR server.
#
# Usage:
#  perl CdrCmd.pl [command-file [host [port]]]
#
# Example:
#  perl CdrCmd.pl CdrCommandSamples.xml mmdb2
#
# Default for host is "localhost"; default for port is 2019.
# If no command-line arguments are given, commands are read from standard
# input.  Command buffer must be valid XML, conforming to the DTD
# CdrCommandSet.dtd, and the top-level element must be <CdrCommandSet>.
# The encoding for the XML must be UTF-8.
#
#----------------------------------------------------------------------

#----------------------------------------------------------------------
# Import required packages.
#----------------------------------------------------------------------
use IO::Socket;

#----------------------------------------------------------------------
# Establish parameters.
#----------------------------------------------------------------------
my $name = $#ARGV >= 0 ? $ARGV[0] : '-';
my $host = $#ARGV >= 1 ? $ARGV[1] : 'localhost';
my $port = $#ARGV >= 2 ? $ARGV[2] : 2019;

#----------------------------------------------------------------------
# Get the commands.
#----------------------------------------------------------------------
open(CMDS, $name) or die "$name: $!";
my $cmds = '';
my $part;
while (read(CMDS, $part, 2048) > 0) { $cmds .= $part; }

#----------------------------------------------------------------------
# Submit them to the CDR server.
#----------------------------------------------------------------------
my $sock = IO::Socket::INET->new(PeerAddr => $host,
                                 PeerPort => $port,
                                 Type     => SOCK_STREAM) or die $@;
$sock->send(pack("N", length($cmds)), 0);
$sock->send($cmds, 0);

#----------------------------------------------------------------------
# Read the server's response.
#----------------------------------------------------------------------
my $resp = '    ';
$sock->recv($resp, 4, 0) or die "can't read response length: $!";
$sock->recv($resp, unpack("N", $resp), 0) or die "can't read response: $!";
chomp $resp;
print "<!-- *** SERVER RESPONSE *** -->\n$resp\n";
