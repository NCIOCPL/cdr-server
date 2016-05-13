non_cdr = set()
for row in open("ftp_vendors.db"):
    record = row.strip()
    if record and not record.startswith("#"):
        fields = record.split(":")
        if len(fields) != 16:
            print len(fields), record
        else:
            if fields[0] != "CDR":
                non_cdr.add(fields[0])
                print record
print "Non-CDR products:"
for product in sorted(non_cdr):
    print "\t%s" % product
