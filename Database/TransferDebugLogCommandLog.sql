USE cdr

-- 1) - rename command_log and debug_log tables 
EXEC sp_rename  "command_log", "command_log_history"
EXEC sp_rename  "debug_log", "debug_log_history"

-- 2)  create new command_log and debug_log tables
CREATE TABLE [dbo].[command_log] (
	[thread] [int] NOT NULL ,
	[received] [datetime] NOT NULL ,
	[command] [text] COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL 
) ON [PRIMARY] TEXTIMAGE_ON [PRIMARY]


CREATE TABLE [dbo].[debug_log] (
	[id] [int] IDENTITY (1, 1) NOT NULL ,
	[thread] [int] NOT NULL ,
	[recorded] [datetime] NOT NULL ,
	[source] [nvarchar] (64) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[msg] [nvarchar] (3800) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL 
) ON [PRIMARY]

Grant Select on command_log to CdrGuest
Grant Select on debug_log to CdrGuest

-- 3) - Transfer data from cdr.command_log_history and cdr.debug_log_history tables to cdrArchive.command_log_history and cdrArchive.debug_log_history tables
INSERT INTO cdrArchive.dbo.command_log_history (
	[thread] ,
	[received] ,
	[command] 
	)
SELECT 	
	[thread] ,
	[received] ,
	[command] 
FROM 	cdr.dbo.command_log_history with(nolock)

INSERT INTO cdrArchive.dbo.debug_log_history (
	[thread],
	[recorded],
	[source],
	[msg]
	)
SELECT 	
	[thread],
	[recorded],
	[source],
	[msg]
FROM 	cdr.dbo.debug_log_history with(nolock)

-- 4) - drop command_log_history and debug_log_history tables in the cdr database
TRUNCATE TABLE cdr.dbo.command_log_history
TRUNCATE TABLE cdr.dbo.debug_log_history

DROP TABLE cdr.dbo.command_log_history
DROP TABLE cdr.dbo.debug_log_history

-- 5) - create indexes on command_log and debug_log tables
ALTER TABLE [dbo].[command_log] WITH NOCHECK ADD 
	CONSTRAINT [command_log_pk] PRIMARY KEY  CLUSTERED 
	(
		[thread],
		[received]
	) WITH  FILLFACTOR = 90  ON [PRIMARY] 

ALTER TABLE [dbo].[debug_log] WITH NOCHECK ADD 
	 PRIMARY KEY  CLUSTERED 
	(
		[id]
	) WITH  FILLFACTOR = 90  ON [PRIMARY] 

CREATE  INDEX [command_log_time] ON [dbo].[command_log]([received]) WITH  FILLFACTOR = 90 ON [PRIMARY]
CREATE  INDEX [debug_log_recorded_idx] ON [dbo].[debug_log]([recorded]) WITH  FILLFACTOR = 90 ON [PRIMARY]

