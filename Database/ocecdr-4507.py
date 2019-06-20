from cdrapi import db

conn = db.connect()
cursor = conn.cursor()
cursor.execute("DROP TABLE glossary_term_audio_request")
conn.commit()
print("glossary_term_audio_request table dropped")
