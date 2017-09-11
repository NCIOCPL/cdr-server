pause Have you backed up the old server?
net stop cdrpublish2
net stop Cdr
pause until you think the services have really stopped
copy CdrServer.exe d:\cdr\Bin
pause until you're ready to restart the CDR service
net start Cdr
pause until you're ready to restart the CDR publishing service
net start cdrpublish2
