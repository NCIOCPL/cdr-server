#----------------------------------------------------------------------
#
# $Id: Makefile,v 1.4 2001-12-22 01:56:37 bkline Exp $
#
# Rules for initializing CDR database at data conversion time.
# Must be run as system administrator.
#
# $Log: not supported by cvs2svn $
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

SQL=CreateCdrLogin.sql\
    CreateDatabase.sql\
    CreateMoreLogins.sql\
    tables.sql\
    procs.sql\
    LoadControlData.sql\
    ShowCounts.sql

#----------------------------------------------------------------------
# Top-level rule.
#----------------------------------------------------------------------
all: $(SQL)
	osql -n -E                -i CreateCdrLogin.sql
	osql -n -U cdr -P ***REMOVED*** -i CreateDatabase.sql
	osql -n -E                -i CreateMoreLogins.sql
	osql -n -U cdr -P ***REMOVED*** -i tables.sql
	osql -n -U cdr -P ***REMOVED*** -i procs.sql
	osql -n -U cdr -P ***REMOVED*** -i LoadControlData.sql
	osql -n -U cdr -P ***REMOVED*** -i ShowCounts.sql
	mv -f LoadControlData.sql LoadControlData.sql.old

#----------------------------------------------------------------------
# Generate SQL script for loading seed data.
#----------------------------------------------------------------------
LoadControlData.sql: base_data_sql.py 
	rm -f LoadControlData.sql
	python base_data_sql.py > tmp.sql
	mv -f tmp.sql LoadControlData.sql

#----------------------------------------------------------------------
# Get the rest from source code control.
#----------------------------------------------------------------------
CreateCdrLogin.sql:
	co CreateCdrLogin.sql

CreateDatabase.sql:
	co CreateDatabase.sql

CreateMoreLogins.sql:
	co CreateMoreLogins.sql

tables.sql:
	co tables.sql

procs.sql:
	co procs.sql

ShowCounts.sql:
	co ShowCounts.sql

base_data_sql.py:
	co base_data_sql.py
