/*
 * $Id$
 *
 * Script to install job to update emailer tracking information.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2004/11/23 16:47:01  bkline
 * Fixed mis-named step.
 *
 * Revision 1.1  2004/06/19 12:27:08  bkline
 * Scheduled update of electronic mailer tracking documents.
 *
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
                        @step_name         = 'Emailer Tracking Update',
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
EXEC sp_add_job         @job_name          = 'Emailer Lookup Tables',
                        @category_name     = 'CDR Jobs'
GO
EXEC sp_add_jobstep     @job_name          = 'Emailer Lookup Tables',
                        @step_name         = 'Upload New Tables',
                        @subsystem         = 'CMDEXEC',
                        @command      = 'd:\cdr\bin\GetEmailerLookupTables.cmd'
GO
EXEC sp_add_jobschedule @job_name          = 'Emailer Lookup Tables',
                        @name              = 'Lookup Refresh Schedule',
                        @freq_type         = 4, -- daily
                        @freq_interval     = 1,
                        @freq_subday_type  = 4, -- minutes
                        @freq_subday_interval = 2,
                        @enabled           = 1
GO
