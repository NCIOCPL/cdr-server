/*
 * $Id: TruncateDebugLogs.sql,v 1.1 2003-07-06 20:23:39 bkline Exp $
 *
 * Drops and recreates command_log and debug_log tables, then
 * compacts the database.
 */
USE cdr
GO

DROP TABLE command_log
GO

DROP TABLE debug_log
GO

CREATE TABLE debug_log
         (id INTEGER        IDENTITY PRIMARY KEY,
      thread INTEGER        NOT NULL,
    recorded DATETIME       NOT NULL,
      source NVARCHAR(64)   NOT NULL,
         msg NVARCHAR(3800) NOT NULL)
GO

CREATE INDEX debug_log_recorded_idx ON debug_log(recorded)
GO

CREATE TABLE command_log
     (thread INTEGER NOT NULL,
    received DATETIME NOT NULL,
     command TEXT NOT NULL,
  CONSTRAINT command_log_pk PRIMARY KEY(thread, received))
CREATE INDEX command_log_time ON command_log(received)
GO

DBCC shrinkdatabase(cdr)
GO
