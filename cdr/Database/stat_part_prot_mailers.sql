SELECT DISTINCT d.doc_id, MAX(v.num)
           FROM doc_version v
           JOIN ready_for_review r
             ON r.doc_id = v.id
           JOIN doc_created d
             ON d.doc_id = r.doc_id
           JOIN query_term s
             ON s.doc_id = d.doc_id
          WHERE d.created >= ?
            AND s.value IN ('Active', 'Approved-Not Yet Active')
            AND s.path = '/InScopeProtocol/ProtocolAdminInfo' +
                         '/CurrentProtocolStatus'
            AND NOT EXISTS (SELECT *
                              FROM query_term src
                             WHERE src.value = 'NCI Liaison ' +
                                               'Office-Brussels'
                               AND src.path  = '/InScopeProtocol' +
                                               '/ProtocolSources' +
                                               '/ProtocolSource' +
                                               '/SourceName'
                               AND src.doc_id = d.doc_id)
            AND NOT EXISTS (SELECT *
                              FROM pub_proc p
                              JOIN pub_proc_doc pd
                                ON p.id = pd.pub_proc
                             WHERE pd.doc_id = d.doc_id
                               AND p.pub_subset = 'Initial Status '
                                                + 'And Participant'
                                                + ' Mailer')
       GROUP BY d.doc_id
