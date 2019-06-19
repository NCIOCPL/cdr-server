from cdrapi import db

conn = db.connect()
cursor = conn.cursor()
values = (
    ('Translation Pending', 12),
    ('Translation in Progress', 15),
    ('Translation Complete', 17),
    ('Translation Peer Review 1 in progress', 23),
    ('Translation Peer Review 1 complete', 26),
    ('Translation Peer Review 2 in progress', 32),
    ('Translation Peer Review 2 complete', 35),
    ('Implement Changes from Peer Reviews', 37),
)
sql = "INSERT INTO glossary_translation_state (value_name, value_pos) VALUES (?, ?)"
for pair in values:
    cursor.execute(sql, pair)
conn.commit()
print("glossary_translation_state rows inserted")
