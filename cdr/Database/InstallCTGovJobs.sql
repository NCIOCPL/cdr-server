/*
 * $Id: InstallCTGovJobs.sql,v 1.2 2004-01-12 19:14:57 bkline Exp $
 *
 * Script to install jobs to download and import protocols from
 * ClinicalTrials.gov.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2003/12/18 21:47:21  bkline
 * Script to install the nightly jobs to download and import trials
 * from ClinicalTrials.gov.
 *
 */
USE msdb
EXEC sp_add_category    @name              = 'CDR Jobs'
GO
EXEC sp_add_job         @job_name          = 'CTGov Nightly Tasks',
                        @category_name     = 'CDR Jobs'
GO
EXEC sp_add_jobstep     @job_name          = 'CTGov Nightly Tasks',
                        @step_name         = 'CTGov Protocol Download',
                        @subsystem         = 'CMDEXEC',
                        @command           = 'd:\cdr\bin\RunCTGovDownload.cmd',
                        @on_success_action = 3, -- go to the next step
                        @on_fail_action    = 3  -- go to the next step
GO
EXEC sp_add_jobstep     @job_name          = 'CTGov Nightly Tasks',
                        @step_name         = 'CTGov Protocol Import',
                        @subsystem         = 'CMDEXEC',
                        @command           = 'd:\cdr\bin\RunCTGovImport.cmd'
GO
EXEC sp_add_jobschedule @job_name          = 'CTGov Nightly Tasks',
                        @name              = 'CTGov Nightly Schedule',
                        @freq_type         = 4, -- daily
                        @freq_interval     = 1,
                        @active_start_time = 30000 -- 3:00 a.m.
GO
