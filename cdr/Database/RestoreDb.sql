USE master
GO
EXEC sp_dboption 'cdr', 'single user', 'TRUE'
GO
RESTORE DATABASE cdr from DISK = '\\CDRDEV\cdr\Database\cdr.bak'
GO
EXEC sp_dboption 'cdr', 'single user', 'FALSE'
GO
USE cdr
GO
EXEC sp_changedbowner 'cdr'
GO
