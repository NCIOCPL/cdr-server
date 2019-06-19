from cdrapi import db

conn = db.connect()
cursor = conn.cursor()
cursor.execute("""\
INSERT INTO media_translation_state (value_name, value_pos)
     VALUES ('2nd Peer-Review and Implement Changes', 95)""")
conn.commit()
print("row inserted")
