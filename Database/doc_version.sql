/*
 * $Id: doc_version.sql,v 1.1 2008-10-08 17:27:02 bkline Exp $
 *
 * Script to create the view for CDR document versions.  This is invoked
 * by db-refresh-postprocess.cmd after refreshing the CDR database on
 * Franck or Mahler from Bach.
 *
 * $Log: not supported by cvs2svn $
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