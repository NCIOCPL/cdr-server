/*
 * $Id: CreateArchive.sql,v 1.1 2009-03-19 21:32:07 ameyer Exp $
 *
 * Database and tables to receive copies of archived command and debug logs.
 *
 * Only needed on production server
 *
 * $Log: not supported by cvs2svn $
 */

USE master
GO
CREATE DATABASE cdrarchive
GO

USE cdrarchive
sp_changedbowner 'cdr'
GO

-- Same as cdr..command_log, but with no constraint or index
CREATE TABLE command_log_history
     (thread INTEGER NOT NULL,
    received DATETIME NOT NULL,
     command TEXT NOT NULL)
GO

-- Same as cdr..debug_log, no constraint, index or identity
CREATE TABLE debug_log_history
         (id INTEGER        NOT NULL,
      thread INTEGER        NOT NULL,
    recorded DATETIME       NOT NULL,
      source NVARCHAR(64)   NOT NULL,
         msg NVARCHAR(3800) NOT NULL)
GO
