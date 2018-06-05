"""
Create new database table for recording PDQ partner notifications

See https://tracker.nci.nih.gov/browse/OCECDR-4100
"""

from argparse import ArgumentParser
from cdrapi import db

# Connect to the database
parser = ArgumentParser()
parser.add_argument("--tier", "-t")
opts = parser.parse_args()
conn = db.connect(tier=opts.tier)
cursor = conn.cursor()

# Fetch notification dates from the old data partner contact table
query = db.Query("data_partner_contact", "email_addr", "notif_date")
query.where("notif_date IS NOT NULL")
query.where("email_addr IS NOT NULL")
rows = query.execute(cursor).fetchall()

# Drop the table if it exists
try:
    cursor.execute("DROP TABLE data_partner_notification")
except:
    pass

# Create the table
cursor.execute("""\
CREATE TABLE data_partner_notification
 (email_addr VARCHAR(64) NOT NULL,
  notif_date DATETIME NOT NULL)""")
cursor.execute("GRANT SELECT ON data_partner_notification TO CdrGuest")
print("New table data_partner_notification created ...")

# Populate the new table with the data collected above
fields = "email_addr", "notif_date"
placeholders = ["?"] * len(fields)
args = "data_partner_notification", ", ".join(fields), ", ".join(placeholders)
insert = "INSERT INTO {} ({}) VALUES ({})".format(*args)
count = 0
for email, date in rows:
    email = email.strip().lower()
    if email:
        cursor.execute(insert, (email, date))
        count += 1
conn.commit()
print("inserted {:d} rows".format(count))
