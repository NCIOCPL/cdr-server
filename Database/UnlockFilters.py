#----------------------------------------------------------------------
#
# $Id: UnlockFilters.py,v 1.1 2003-07-06 19:44:43 bkline Exp $
#
# Script for checking filters copied from BACH back in on MAHLER.
#
# $Log: not supported by cvs2svn $
#----------------------------------------------------------------------
import cdrdb

conn = cdrdb.connect()
cursor = conn.cursor()
cursor.execute("""\
    UPDATE checkout
       SET dt_in = GETDATE()
      FROM checkout c
      JOIN document d
        ON d.id = c.id
      JOIN doc_type t
        ON t.id = d.doc_type
     WHERE t.name = 'Filter'
       AND c.dt_in IS NULL""")
conn.commit()
