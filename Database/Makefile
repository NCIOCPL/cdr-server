#----------------------------------------------------------------------
#
# $Id: Makefile,v 1.7 2002-04-20 12:20:27 bkline Exp $
#
# Rules for initializing CDR database at data conversion time.
# Must be run as system administrator.
#
# $Log: not supported by cvs2svn $
# Revision 1.6  2002/04/20 02:53:33  bkline
# Switched version control rules from RCS to CVS.
#
# Revision 1.5  2001/12/24 01:00:28  bkline
# Separated creation of logins to end of process.
#
# Revision 1.4  2001/12/22 01:56:37  bkline
# Added rules to check scripts out of source code control.
#
# Revision 1.3  2001/12/19 21:04:42  bkline
# Replaced rename command with 'mv -f'; moved LoadControlData.sql to
# LoadControlData.sql.old after use.
#
# Revision 1.2  2001/12/19 20:51:44  bkline
# Added intermediate output file for base_data_sql.py to make sure we
# don't use a partial output set from a previous failed run.
#
# Revision 1.1  2001/12/19 20:47:12  bkline
# Initial revision
#
#----------------------------------------------------------------------

CVSUPD=cvs update
SQL=CreateLogins.sql\
    tables.sql\
    procs.sql\
    LoadControlData.sql\
    ShowCounts.sql

#----------------------------------------------------------------------
# Top-level rule.
#----------------------------------------------------------------------
all: cvsupdate
	$(MAKE) _all

cvsupdate:
	$(CVSUPD)

_all: $(SQL)
	osql -n -E -i tables.sql
	osql -n -E -i procs.sql
	osql -n -E -i LoadControlData.sql
	osql -n -E -i CreateLogins.sql
	osql -n -E -i ShowCounts.sql
	mv -f LoadControlData.sql LoadControlData.sql.old

#----------------------------------------------------------------------
# Generate SQL script for loading seed data.
#----------------------------------------------------------------------
LoadControlData.sql: base_data_sql.py 
	rm -f LoadControlData.sql
	python base_data_sql.py > tmp.sql
	mv -f tmp.sql LoadControlData.sql
