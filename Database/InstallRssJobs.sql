/*
 * $Id$
 *
 * Script to install jobs to download and import RSS protocol info from
 * CTEP.
 *
 * $Log: not supported by cvs2svn $
 */
USE msdb
EXEC sp_add_category    @name              = 'CDR Jobs'
GO
EXEC sp_add_job         @job_name          = 'RSS Nightly Tasks',
                        @category_name     = 'CDR Jobs'
GO
EXEC sp_add_jobstep     @job_name          = 'RSS Nightly Tasks',
                        @step_name         = 'RSS Protocol Download',
                        @subsystem         = 'CMDEXEC',
                        @command           = 'd:\cdr\bin\RunRssDownload.cmd'
GO
EXEC sp_add_jobschedule @job_name          = 'RSS Nightly Tasks',
                        @name              = 'RSS Nightly Schedule',
                        @freq_type         = 4, -- daily
                        @freq_interval     = 1,
                        @active_start_time = 20000 -- 2:00 a.m.
GO
