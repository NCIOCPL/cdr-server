REM $Id: db-refresh-postprocess.cmd,v 1.1 2008-09-25 14:17:14 bkline Exp $
REM
REM Run this after restoring the cdr database on Franck or Mahler
REM from a backup taken on Bach.
osql -E < doc_version.sql
osql -E < CreateLogins.sql
