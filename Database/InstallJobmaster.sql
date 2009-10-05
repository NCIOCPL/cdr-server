USE msdb; 
GO

BEGIN TRANSACTION            
  DECLARE @JobID      BINARY(16)  
  DECLARE @ReturnCode INT    
  SELECT  @ReturnCode = 0     

-- Create the Jobmaster Category if it doesn't exist already
-- ---------------------------------------------------------
  IF (SELECT COUNT(*) 
        FROM syscategories 
       WHERE name = N'CDR Jobmaster') < 1 
    EXEC sp_add_category 
        @class        = N'Job',
        @type         = N'Local',
        @name         = N'CDR Jobmaster';

-- Delete the job with the same name (if it exists)
-- ------------------------------------------------
  SELECT @JobID = job_id     
    FROM sysjobs    
   WHERE name = N'Daily Publishing'

  IF (@JobID IS NOT NULL)    
  BEGIN  
    EXEC sp_delete_job 
        @job_name = N'Daily Publishing' 
    SELECT @JobID = NULL
  END 


-- Recreate the job
-- ----------------
  EXEC     
    @ReturnCode        = sp_add_job   
    @job_id            = @JobID OUTPUT , 
    @owner_login_name  = N'sa',                   -- only sa has permission
    @job_name          = N'Daily Publishing',
    @category_name     = N'CDR Jobmaster',
    @description       = N'Nightly publishing job - weeknights only';
 
  IF (@ReturnCode <> 0)
     GOTO QuitWithRollback

-- Add the jobserver
-- -----------------
  EXEC
    @ReturnCode        = sp_add_jobserver
    @job_name          = N'Daily Publishing',
    @server_name       = N'(Local)';

  IF (@ReturnCode <> 0)
     GOTO QuitWithRollback

-- Add the job steps to the Job
-- ----------------------------
  EXEC 
    @ReturnCode        = sp_add_jobstep
    @job_id            = @JobID,
    @step_id           = 1,
    @step_name         = 'Jobmaster Starting',
    @command           = 'd:\python\python d:\home\venglisch\cdr\publishing\SuccessEmail.py "Daily Publishing Started" "Job started successfully"',
    @subsystem         = 'CMDEXEC',
    @on_success_action = 3, -- Go to next step
    @on_fail_action    = 4, -- go to step indicated
    @on_fail_step_id   = 7;
 
  IF (@ReturnCode <> 0)
     GOTO QuitWithRollback

-- Submit the nightly publishing job
-- ---------------------------------
  EXEC 
    @ReturnCode        = sp_add_jobstep     
    @job_id            = @JobID,
    @step_id           = 2,
    @step_name         = 'Submit Pub Job',
    @command           = 'd:\python\python d:\home\venglisch\cdr\Publishing\SubmitPubJob.py "daily"',
    @subsystem         = 'CMDEXEC',
    @on_success_action = 3,  -- Got to next step
    @on_fail_action    = 4,  -- go to step indicated
    @on_fail_step_id   = 8; 
 
  IF (@ReturnCode <> 0)
     GOTO QuitWithRollback

-- Submit the job to FTP the data (We only need this for the weekly but 
-- for testing purposes we are running this for the nightly
-- ---------------------------------------------------------------------
  EXEC 
    @ReturnCode        = sp_add_jobstep     
    @job_id            = @JobID,
    @step_id           = 3,
    @step_name         = 'FTP Pub Data',
    @command           = 'd:\python\python d:\home\venglisch\cdr\Publishing\Copy2Ftp.py "--testmode" "--nightly"',
    @subsystem         = 'CMDEXEC',
    @on_success_action = 3,  -- Got to next step
    @on_fail_action    = 4,  -- go to step indicated
    @on_fail_step_id   = 8; 
 
  IF (@ReturnCode <> 0)
     GOTO QuitWithRollback

-- Creating the CTGovExport Data to be exported to the NLM
-- -------------------------------------------------------
  EXEC
    @ReturnCode        = sp_add_jobstep
    @job_id            = @JobID,
    @step_id           = 4,
    @step_name         = 'Create CTGovExport Data',
    @command           = 'd:\home\venglisch\cdr\Publishing\Ref909.cmd',
    @subsystem         = 'CMDEXEC',
    @on_success_action = 3, -- go to next step,
    @on_fail_action    = 4, -- go to step indicated
    @on_fail_step_id   = 7;

  IF (@ReturnCode <> 0)
     GOTO QuitWithRollback

-- Copying the CTGovExport Data from the CDR server to the FTP Server
-- ------------------------------------------------------------------
  EXEC
    @ReturnCode        = sp_add_jobstep
    @job_id            = @JobID,
    @step_id           = 5,
    @step_name         = 'Copy CTGovExport Data',
    @command           = 'd:\python\python d:\home\venglisch\cdr\Publishing\Ftp909.py',
    @subsystem         = 'CMDEXEC',
    @on_success_action = 3, -- go to next step,
    @on_fail_action    = 4, -- go to step indicated
    @on_fail_step_id   = 7;

  IF (@ReturnCode <> 0)
     GOTO QuitWithRollback

-- Sending Email after successful completion
-- -----------------------------------------
  EXEC 
    @ReturnCode        = sp_add_jobstep     
    @job_id            = @JobID,
    @step_id           = 6,
    @step_name         = 'Success CTGovExport Notification',
    @command           = 'd:\python\python d:\home\venglisch\cdr\Publishing\SuccessEmail.py Step_Completed "Ref 909"',
    @subsystem         = 'CMDEXEC',
    @on_success_action = 1, -- quit with success
    @on_fail_action    = 2;  -- quit with failure
 
  IF (@ReturnCode <> 0)
     GOTO QuitWithRollback

-- Sending Email after failed completion
-- -------------------------------------
  EXEC 
    @ReturnCode        = sp_add_jobstep     
    @job_id            = @JobID,
    @step_id           = 7,
    @step_name         = 'Error Email Notification',
    @command           = 'd:\python\python d:\home\venglisch\cdr\Publishing\ErrorEmail.py Step_Failed Ref_909',
    @subsystem         = 'CMDEXEC',
    @on_success_action = 1, -- quit with success
    @on_fail_action    = 4,  -- quit with failure
    @on_fail_step_id   = 8;
 
  IF (@ReturnCode <> 0)
     GOTO QuitWithRollback

  EXEC
    @ReturnCode        = sp_add_jobstep
    @job_id            = @JobID,
    @step_id           = 8,
    @step_name         = 'Error',
    @command           = 'd:\python\python d:\home\venglisch\cdr\Publishing\ErrorEmail.py ErrorEmailFailed Ref_909',
    @subsystem         = 'CMDEXEC',
    @on_success_action = 1, -- go to next step,
    @on_fail_action    = 2; -- go to step indicated

  IF (@ReturnCode <> 0)
     GOTO QuitWithRollback

  EXEC
    @ReturnCode        = sp_add_jobschedule
    @job_id            = @JobID,
    @name              = 'MFP',
    @enabled           = 1,
    @freq_type         = 4,   -- Daily
    @freq_interval     = 1,  -- Monday, Tuesday, Wednesday
    @freq_recurrence_factor = 1,
    @active_start_time = 080000;
 
  IF (@ReturnCode <> 0)
     GOTO QuitWithRollback

COMMIT TRANSACTION

GOTO EndSave
QuitWithRollback:
   IF (@@TRANCOUNT > 0)
      ROLLBACK TRANSACTION

EndSave:
