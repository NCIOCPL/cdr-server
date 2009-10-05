USE cdr
GO
SET NOCOUNT ON
GO
SELECT 'usr', COUNT(*) FROM usr
GO
SELECT 'format', COUNT(*) FROM format
GO
SELECT 'doc_type', COUNT(*) FROM doc_type
GO
SELECT 'action', COUNT(*) FROM action
GO
SELECT 'grp', COUNT(*) FROM grp
GO
SELECT 'doc_status', COUNT(*) FROM doc_status
GO
SELECT 'active_status', COUNT(*) FROM active_status
GO
SELECT 'grp_action', COUNT(*) FROM grp_action
GO
SELECT 'grp_usr', COUNT(*) FROM grp_usr
GO
SELECT 'link_type', COUNT(*) FROM link_type
GO
SELECT 'link_xml', COUNT(*) FROM link_xml
GO
SELECT 'link_target', COUNT(*) FROM link_target
GO
SELECT 'link_prop_type', COUNT(*) FROM link_prop_type
GO
SELECT 'link_properties', COUNT(*) FROM link_properties
GO
SELECT 'query_term_rule', COUNT(*) FROM query_term_rule
GO
SELECT 'query_term_def', COUNT(*) FROM query_term_def
GO
SELECT 'dev_task_status', COUNT(*) FROM dev_task_status
GO
SELECT 'issue_priority', COUNT(*) FROM issue_priority
GO
SELECT 'issue_user', COUNT(*) FROM issue_user
GO
SELECT 'dev_task', COUNT(*) FROM dev_task
GO
SELECT 'issue', COUNT(*) FROM issue
GO
SELECT 'report_task', COUNT(*) FROM report_task
GO
SELECT 'document', COUNT(*) FROM document
GO
