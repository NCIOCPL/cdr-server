#!/usr/bin/env python

from cdrapi import db

conn = db.connect()
cursor = conn.cursor()
cursor.execute("ALTER TABLE term_audio_mp3 ADD reuse_media_id INT NULL")
conn.commit()
