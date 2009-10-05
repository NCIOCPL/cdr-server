# Creates script to expire old users.  Originally this list includes
# statements for all the users.  Edit the script by hand to remove
# the statements for users who should remain active.
import cdrdb
conn = cdrdb.connect()
curs = conn.cursor()
curs.execute("SELECT name FROM usr ORDER BY name")
for row in curs.fetchall(): 
    print "UPDATE usr SET expired = '2002-01-01' WHERE name = '%s';" % row[0]
