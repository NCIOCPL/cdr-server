#----------------------------------------------------------------------
# Create time-stamped backups of the three PDQ data partner DB tables.
#----------------------------------------------------------------------
import cdrdb
import datetime

TABLES = ("data_product", "data_partner_org", "data_partner_contact")
cursor = cdrdb.connect("CdrGuest").cursor()
time_stamp = datetime.datetime.now().strftime("%Y%m%d%H%M%S")
for table_name in TABLES:
    file_name = "%s-%s.txt" % (table_name, time_stamp)
    fp = open(file_name, "w")
    cursor.execute("SELECT * FROM %s" % table_name)
    fp.write("%s\n" % repr([desc[0] for desc in cursor.description]))
    for row in cursor.fetchall():
        fp.write("%s\n" % repr(row))
    fp.close()
    print "wrote", file_name
