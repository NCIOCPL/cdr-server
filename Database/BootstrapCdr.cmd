osql -n -E                -i CreateCdrLogin.sql
osql -n -U cdr -P ***REMOVED*** -i CreateDatabase.sql
osql -n -E                -i CreateMoreLogins.sql
osql -n -U cdr -P ***REMOVED*** -i tables.sql
osql -n -U cdr -P ***REMOVED*** -i procs.sql
osql -n -U cdr -P ***REMOVED*** -i LoadControlData.sql
osql -n -U cdr -P ***REMOVED*** -i ShowCounts.sql

