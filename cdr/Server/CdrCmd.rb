#!/usr/local/bin/ruby
#----------------------------------------------------------------------
#
# $Id: CdrCmd.rb,v 1.2 2008-02-24 23:04:40 bkline Exp $
#
# Test client (Ruby version) for sending commands to CDR server.
#
# Usage:
#  ruby CdrCmd.rb [command-file [host [port]]]
#
# Example:
#  ruby CdrCmd.rb CdrCommandSamples.xml mmdb2
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
require 'socket'

#----------------------------------------------------------------------
# Establish parameters.
#----------------------------------------------------------------------
name = if ARGV.size > 0 then ARGV.shift else '' end
host = if ARGV.size > 0 then ARGV.shift else 'mahler.nci.nih.gov' end
port = if ARGV.size > 0 then ARGV.shift else 2019 end


#----------------------------------------------------------------------
# Get the commands.
#----------------------------------------------------------------------
file = if name == '' then $stdin else open(name) end
cmds = file.read()

#----------------------------------------------------------------------
# Submit them to the CDR server.
#----------------------------------------------------------------------
sock = TCPSocket.open(host, port)
sock.send([cmds.length()].pack("N"), 0)
sock.send(cmds, 0)

#----------------------------------------------------------------------
# Read the server's response.
#----------------------------------------------------------------------
resp = sock.recv(sock.recv(4, 0).unpack("N")[0], 0).chomp()
$stdout << "<!-- *** SERVER RESPONSE *** -->\n" << resp << "\n"
