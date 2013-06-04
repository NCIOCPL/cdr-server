------------------------------------------------------------------
-- Remove stored passwords in the CDR
--
-- Run this script any time a CDR database is imported into the CBIIT
-- environment from the old OCE environment where plain text passwords
-- are stored.
-- 
-- NOTE: Comment or uncomment the following lines depending on how
--       confident you are in what you are doing.
--   DROP TABLE xtemp_pw_backup
--   ALTER TABLE DROP password
------------------------------------------------------------------


------------------------------------------------------------------
-- Optional backup of password column
------------------------------------------------------------------
USE cdr
GO

BEGIN TRANSACTION
CREATE TABLE xtemp_pw_backup (
    name VARCHAR(32), 
    pw   VARCHAR(32)
)
GO

-- Create the backup
INSERT INTO xtemp_pw_backup (name, pw) 
SELECT u.name, u.password FROM usr u
GO

-- Verify it
SELECT x.name, x.pw, u.password
  FROM xtemp_pw_backup x, usr u 
 WHERE x.name = u.name 
 ORDER BY x.name
GO

-- Transaction
COMMIT TRANSACTION

-- If we're done with the backup
-- DROP TABLE xtemp_pw_backup

------------------------------------------------------------------
-- Script to modify the CDR database for password hashing
------------------------------------------------------------------
USE cdr
GO

-- Transaction
BEGIN TRANSACTION

-- Add column to hold the hashed password
-- The default value makes the userid unusable until a real pw is established
ALTER TABLE usr ADD hashedpw VARBINARY(2048) NOT NULL DEFAULT 0x123ABC654
GO

-- Add column to hold the number of failed login tries since last success
ALTER TABLE usr ADD login_failed INT NOT NULL DEFAULT 0
GO

-- Convert all of the passwords from plain text to hashe
UPDATE usr SET hashedpw = HASHBYTES('SHA1', password)
GO

-- Transaction
COMMIT TRANSACTION


------------------------------------------------------------------
-- Remove the password column from the usr table
-- 
-- If there are constraints, the DB admin may have to find 
-- the constraints and drop them before dropping the table.
------------------------------------------------------------------
USE cdr
ALTER TABLE usr DROP password
