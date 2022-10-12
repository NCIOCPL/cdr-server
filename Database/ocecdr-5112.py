#!/usr/bin/env python

"""Prune obsolete stored procedures.

For Ohm release.
"""

from argparse import ArgumentParser
from cdrapi import db

PROCS = (
    "cdr_coop_group_report",
    "cdr_get_tree_context",
    "find_linked_docs",
    "get_participating_orgs",
    "get_prot_person_connections",
    "select_changed_non_active_protocols",
)

parser = ArgumentParser()
parser.add_argument("--tier")
opts = parser.parse_args()
conn = db.connect(tier=opts.tier)
cursor = conn.cursor()
for name in PROCS:
    print(f"dropping {name} ...")
    cursor.execute(f"""\
IF EXISTS (SELECT *
             FROM sysobjects
            WHERE name = '{name}'
              AND type = 'P')
    DROP PROCEDURE {name}""")
conn.commit()
cursor.execute("SELECT * FROM SYS.TRIGGERS WHERE NAME = 'cdr_mod_doc'")
rows = cursor.fetchall()
if not rows:
    print("installing trigger 'cdr_mod_doc' ...")
    cursor.execute("""\
/*
 * Make sure a CDR document is not marked as 'D'eleted if there's a row
 * in the external_map table which maps to it.
 */
CREATE TRIGGER cdr_mod_doc ON all_docs
FOR UPDATE
AS
    IF UPDATE(active_status)
    BEGIN
        IF EXISTS (SELECT i.id
                     FROM cdr..external_map m
                     JOIN inserted i
                       ON m.doc_id = i.id
                    WHERE i.active_status = 'D')
        BEGIN
            RAISERROR('Attempt to delete document in external_map table', 16, 1)
            ROLLBACK TRANSACTION
        END
    END
""".replace("\n", "\r\n"))
    conn.commit()
