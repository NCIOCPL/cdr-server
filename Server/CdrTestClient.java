/*
 * $Id$
 *
 * Test client (Java version) for sending commands to CDR server.
 * 
 * Usage:
 *  java CdrTestClient [command-file [host [port]]]
 *
 * Example:
 *  java CdrTestClient CdrCommandSamples.xml mmdb2
 *
 * Default for host is "localhost"; default for port is 2019.
 * If no command-line arguments are given, commands are read from standard
 * input.  Command buffer must be valid XML, conforming to the DTD
 * CdrCommandSet.dtd, and the top-level element must be <CdrCommandSet>.
 * The encoding for the XML must be UTF-8.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2000/04/17 03:15:23  bkline
 * Allowed "-" to mean standard input on command line.
 *
 * Revision 1.1  2000/04/13 17:12:48  bkline
 * Initial revision
 *
 */

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.FileInputStream;
import java.io.InputStream;
import java.net.Socket;

class CdrTestClient {
    public static final int CDR_PORT = 2019;
    public static void main(String av[]) {
        try {
            DataInputStream cmdFile;
            if (av.length > 0 && !av[0].equals("-"))
                cmdFile = new DataInputStream(new FileInputStream(av[0]));
            else
                cmdFile = new DataInputStream(System.in);
            byte[] cmds = readBytes(cmdFile);
            String host = av.length > 1 ? av[1] : "localhost";
            int    port = av.length > 2 ? Integer.parseInt(av[2]) : CDR_PORT;
            Socket           s   = new java.net.Socket(host, port);
            DataInputStream  in  = new DataInputStream(s.getInputStream());
            DataOutputStream out = new DataOutputStream(s.getOutputStream());
            out.writeInt(cmds.length);
            out.write(cmds);
            int nRspBytes = in.readInt();
            byte[] rspBytes = new byte[nRspBytes];
            in.readFully(rspBytes);
            String response = utf8ToUtf16(rspBytes);
            System.out.println("<!-- Server response: -->\n" + response);
        } catch (Exception e) { System.err.println("error: " + e); }
    }
    private static byte[] readBytes(InputStream inputStream)
        throws java.io.IOException {
        int nBytes   = 0;
        byte[] bytes = new byte[nBytes];
        byte[] buf   = new byte[8192];
        for (;;) {
            int n = inputStream.read(buf);
            if (n < 0)
                return bytes;
            if (n > 0) {
                byte[] tmp = new byte[nBytes + n];
                if (nBytes > 0)
                    System.arraycopy(bytes, 0, tmp, 0, nBytes);
                System.arraycopy(buf, 0, tmp, nBytes, n);
                bytes = tmp;
                nBytes += n;
            }
        }
    }
    private static String utf8ToUtf16(byte[] bytes) {
        int utflen = bytes.length;
        char str[] = new char[utflen];
        int count = 0;
        int strlen = 0;
        while (count < utflen) {
            byte b = bytes[count++];
            if (b < 0x80)
                str[strlen++] = (char)b;
            else if ((b & 0xE0) == 0xC0) {
                byte b2 = bytes[count++];
                str[strlen++] = (char)(((b & 0x1F) << 6)
                              | (b2 & 0x3F));
            }
            else {
                byte b2 = bytes[count++];
                byte b3 = bytes[count++];
                str[strlen++] = (char)(((b & 0x0F) << 12)
                              | ((b2 & 0x3F) << 6)
                              | (b3 & 0x3F));
            }
        }
        return new String(str, 0, strlen);
    }
}
