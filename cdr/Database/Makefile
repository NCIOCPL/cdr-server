#----------------------------------------------------------------------
#
# $Id: Makefile,v 1.1 2001-12-19 20:47:12 bkline Exp $
#
# Rules for initializing CDR database at data conversion time.
# Must be run as system administrator.
#
# $Log: not supported by cvs2svn $
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

#----------------------------------------------------------------------
# Generate SQL script for loading seed data.
#----------------------------------------------------------------------
LoadControlData.sql: base_data_sql.py 
	python base_data_sql.py > LoadControlData.sql
