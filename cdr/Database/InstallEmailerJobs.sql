/*
 * $Id: InstallEmailerJobs.sql,v 1.1 2004-06-19 12:27:08 bkline Exp $
 *
 * Script to install job to update emailer tracking information.
 *
 * $Log: not supported by cvs2svn $
 */
USE msdb
/* - Already done by InstallCTGovJobs.sql.
EXEC sp_add_category    @name              = 'CDR Jobs'
xGO
*/
EXEC sp_add_job         @job_name          = 'Emailer Tracking Update',
                        @category_name     = 'CDR Jobs'
GO
EXEC sp_add_jobstep     @job_name          = 'Emailer Tracking Update',
                        @step_name         = 'CTGov Protocol Download',
                        @subsystem         = 'CMDEXEC',
                        @command           = 'd:\cdr\bin\UpdateTrackers.cmd'
GO
EXEC sp_add_jobschedule @job_name          = 'Emailer Tracking Update',
                        @name              = 'Tracking Update Schedule',
                        @freq_type         = 4, -- daily
                        @freq_interval     = 1,
                        @freq_subday_type  = 4, -- minutes
                        @freq_subday_interval = 1,
                        @enabled           = 1
GO
