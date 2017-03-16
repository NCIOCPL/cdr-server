/*
 * Script to create the view for CDR document versions.  This is invoked
 * after refreshing the CDR database on the lower tiers from a backup of
 * the production database.
 */

USE cdr
GO
DROP VIEW doc_version
GO
CREATE VIEW doc_version
AS
         SELECT v.id, v.num, v.dt, v.updated_dt, v.usr,
                v.val_status, v.val_date,
                v.publishable, v.doc_type, v.title,
                xml = CASE
                          WHEN v.xml IS NOT NULL THEN v.xml
                          ELSE a.xml
                      END,
                comment
           FROM all_doc_versions v
LEFT OUTER JOIN cdr_archived_versions.dbo.doc_version_xml a
             ON a.id = v.id
            AND a.num = v.num
GO
