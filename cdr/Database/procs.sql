/*
 * $Id: procs.sql,v 1.12 2002-06-04 20:05:48 bkline Exp $
 *
 * Stored procedures for CDR.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.11  2002/06/03 22:11:58  bkline
 * Added missing frag_id for principal investigators.
 *
 * Revision 1.10  2002/03/20 14:02:38  bkline
 * Added DROP statement for cdr_newly_published_trials procedure.
 *
 * Revision 1.9  2002/03/20 05:46:16  bkline
 * Added procedure cdr_newly_published_trails.
 *
 * Revision 1.8  2002/01/29 20:22:41  bkline
 * Added 'use cdr' command.
 *
 * Revision 1.7  2002/01/02 21:58:54  bkline
 * Added depth paramater for term tree proc.
 *
 * Revision 1.6  2001/12/19 20:09:47  bkline
 * Added some new procedures.
 *
 * Revision 1.5  2001/10/12 22:22:01  bkline
 * Added prot_init_mailer_docs.
 *
 * Revision 1.4  2001/09/04 20:30:28  bkline
 * Changed MemberOfCooperativeGroup to MemberOfCooperativeGroups to reflect
 * switch made by Ark.
 *
 * Revision 1.3  2001/09/04 19:38:06  bkline
 * Added procedure get_participating_orgs.
 *
 * Revision 1.2  2001/04/08 19:20:53  bkline
 * Added statement terminators.
 *
 * Revision 1.1  2001/04/08 19:08:34  bkline
 * Initial revision
 */

/*
 * Switch to the right database.
 */
use cdr
GO

/*
 * Drop old versions.
 */
IF EXISTS (SELECT *
             FROM sysobjects
            WHERE name = 'cdr_get_term_tree'
              AND type = 'P')
    DROP PROCEDURE cdr_get_term_tree
GO
IF EXISTS (SELECT *
             FROM sysobjects
            WHERE name = 'get_participating_orgs'
              AND type = 'P')
    DROP PROCEDURE get_participating_orgs
GO
IF EXISTS (SELECT *
             FROM sysobjects
            WHERE name = 'prot_init_mailer_docs'
              AND type = 'P')
    DROP PROCEDURE prot_init_mailer_docs
GO
IF EXISTS (SELECT *
             FROM sysobjects
            WHERE name = 'cdr_get_tree_context'
              AND type = 'P')
    DROP PROCEDURE cdr_get_tree_context
GO
IF EXISTS (SELECT *
             FROM sysobjects
            WHERE name = 'cdr_newly_published_trials'
              AND type = 'P')
    DROP PROCEDURE cdr_newly_published_trials
GO
IF EXISTS (SELECT *
             FROM sysobjects
            WHERE name = 'cdr_coop_group_report'
              AND type = 'P')
    DROP PROCEDURE cdr_coop_group_report
GO


/**
 * Finds all the parents and children of a Term document in the CDR.  Two
 * result sets are created.  The first contains document id pairs for
 * child-parent relationships.  The second result set contains the document id
 * and title for each id (child or parent) in the first result set.
 *
 *  @param  doc_id      primary key for Term document whose children and
 *                      parents are requested.
 *  @param  depth       number of levels (usually 1) to descend for child
 *                      terms.
 */
CREATE PROCEDURE cdr_get_term_tree
    @doc_id INT,
    @depth  INT
AS
    -- Used for loop control.
    DECLARE @nrows   INT
    DECLARE @nlevels INT

    /*
     * Create temporary tables for the parents and children of the caller's
     * Term document.
     */
    CREATE TABLE #parents(child INT, parent INT)
    CREATE INDEX idx_pc ON #parents(child)
    CREATE INDEX idx_pp ON #parents(parent)
    CREATE TABLE #children(child INT, parent INT)
    CREATE INDEX idx_cc ON #children(child)
    CREATE INDEX idx_cp ON #children(parent)

    /*
     * Insert the immediate parents of the caller's term in the temporary
     * #parents table.
     */
    INSERT INTO #parents(child, parent)
         SELECT child, parent
           FROM term_kids
          WHERE child = @doc_id

    /*
     * Continue gathering parents until we find no more.
     */
    SELECT @nrows = @@ROWCOUNT
    WHILE @nrows > 0
    BEGIN
            INSERT INTO #parents(child, parent)
        SELECT DISTINCT tk.child, tk.parent
                   FROM term_kids tk, #parents p
                  WHERE tk.child = p.parent
                    AND p.parent NOT IN (SELECT child
                                           FROM #parents)
        SELECT @nrows = @@ROWCOUNT
    END

    /*
     * Insert the immediate children of the caller's term in the temporary
     * #children table.
     */
    INSERT INTO #children(child, parent)
         SELECT child, parent
           FROM term_kids
          WHERE parent = @doc_id

    /*
     * Continue gathering children to depth requested until we find no more.
     */
    SELECT @nrows   = @@ROWCOUNT
    SELECT @nlevels = 1
    WHILE @nrows > 0 AND @nlevels < @depth
    BEGIN
            INSERT INTO #children(child, parent)
        SELECT DISTINCT tk.child, tk.parent
                   FROM term_kids tk, #children c
                  WHERE tk.parent = c.child
                    AND c.child NOT IN (SELECT parent
                                          FROM #children)
        SELECT @nrows = @@ROWCOUNT
        SELECT @nlevels = @nlevels + 1
    END

    /*
     * Generate the first result set, containing all the child-parent
     * relationships we have found.
     */
    SELECT DISTINCT * 
               FROM #parents
              UNION
             SELECT * 
               FROM #children

    /*
     * Generate the second result set, so the caller will be able to find the
     * names of all the parent and child terms in the first result set.
     */
    SELECT id, title
      FROM document
     WHERE id IN (SELECT DISTINCT child  FROM #children
                            UNION
                           SELECT parent FROM #children
                            UNION
                           SELECT child  FROM #parents
                            UNION
                           SELECT parent FROM #parents)

    -- Clean up after ourselves.
    DROP TABLE #parents
    DROP TABLE #children
GO

/*
 * Finds participating organizations and their principal investigators
 * for a protocol, given the ID of the lead organization.
 */
CREATE PROCEDURE get_participating_orgs
    @lead_org INTEGER
AS

    SELECT DISTINCT doc_id AS id, path
               INTO #po
               FROM query_term
              WHERE int_val = @lead_org
                AND path = '/Organization' +
                           '/OrganizationAffiliations' +
                           '/MemberOfAdHocGroup/@cdr:ref'

        INSERT INTO #po(id, path)
    SELECT DISTINCT qt1.doc_id, qt1.path
               FROM query_term qt1
               JOIN query_term qt2
                 ON qt1.doc_id  = qt2.doc_id
              WHERE qt1.int_val = @lead_org
                AND qt2.value   = 'Yes'
                AND qt1.path LIKE '/Organization' +
                                  '/OrganizationAffiliations' +
                                  '/MemberOfCooperativeGroups' +
                                  '/%MemberOf' +
                                  '/CooperativeGroup/@cdr:ref'
                AND qt2.path LIKE '/Organization' +
                                  '/OrganizationAffiliations' +
                                  '/MemberOfCooperativeGroups' +
                                  '/%MemberOf' +
                                  '/ProtocolParticipation'
                AND LEFT(qt1.node_loc, 12) =
                    LEFT(qt2.node_loc, 12)

    SELECT DISTINCT #po.id AS po, d.id, d.title, frag.value AS frag_id
               INTO #pi
               FROM document d
               JOIN query_term po
                 ON po.doc_id = d.id
               JOIN #po
                 ON #po.id = po.int_val
               JOIN query_term lo
                 ON lo.doc_id = po.doc_id
                AND LEFT(lo.node_loc, 8) = LEFT(po.node_loc, 8)
               JOIN query_term role
                 ON role.doc_id = po.doc_id
                AND LEFT(role.node_loc, 12) = 
                    LEFT(lo.node_loc, 12)
               JOIN query_term frag
                 ON frag.doc_id = po.doc_id
                AND LEFT(frag.node_loc, 8) = LEFT(po.node_loc, 8)
              WHERE po.path    = '/Person' +
                                 '/PersonLocations' +
                                 '/OtherPracticeLocation' +
                                 '/OrganizationLocation/@cdr:ref'
                AND lo.path    = '/Person' +
                                 '/PersonLocations' +
                                 '/OtherPracticeLocation' +
                                 '/ComplexAffiliation' +
                                 '/Organization/@cdr:ref'
                AND role.path  = '/Person' +
                                 '/PersonLocations' +
                                 '/OtherPracticeLocation' +
                                 '/ComplexAffiliation' +
                                 '/RoleAtAffiliatedOrganization'
                AND frag.path  = '/Person/PersonLocations' + 
                                 '/OtherPracticeLocation/@cdr:id'
                AND role.value = 'Principal Investigator'
                AND lo.int_val = @lead_org

             SELECT po.id, po.title, #po.path, pi.id, pi.title, pi.frag_id
               FROM document po
               JOIN #po
                 ON #po.id = po.id
    LEFT OUTER JOIN #pi pi
                 ON pi.po = po.id
           ORDER BY po.title,
                    po.id,
                    #po.path,
                    pi.title,
                    pi.id
GO

/*
 * Needed because of a bug in Microsoft's ADO, which chokes on the
 * use of a query parameter placeholder following a nested query.
 */
CREATE PROCEDURE prot_init_mailer_docs
    @start_date VARCHAR(50)
AS
    SELECT DISTINCT d.doc_id, MAX(v.num) AS version
               FROM doc_version v
               JOIN ready_for_review d
                 ON d.doc_id = v.id
               JOIN query_term s
                 ON s.doc_id = d.doc_id
              WHERE s.value IN ('Active', 'Approved-Not Yet Active')
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
                AND EXISTS (SELECT *
                              FROM pub_proc_doc p
                             WHERE p.doc_id = d.doc_id)
                AND (SELECT MIN(dt)
                       FROM audit_trail a
                      WHERE a.document = d.doc_id) > @start_date
           GROUP BY d.doc_id
GO

CREATE PROCEDURE cdr_get_tree_context
    @doc_id INT
AS
    DECLARE @nrows INT
    CREATE TABLE #parents(child INT, parent INT)
    CREATE INDEX idx_pc ON #parents(child)
    CREATE INDEX idx_pp ON #parents(parent)
    CREATE TABLE #children(child INT, parent INT)
    CREATE INDEX idx_cc ON #children(child)
    CREATE INDEX idx_cp ON #children(parent)
    INSERT INTO #parents(child, parent)
         SELECT child, parent
           FROM term_parents
          WHERE child = @doc_id
    SELECT @nrows = @@ROWCOUNT
    WHILE @nrows > 0
    BEGIN
            INSERT INTO #parents(child, parent)
        SELECT DISTINCT tp.child, tp.parent
                   FROM term_parents tp, #parents p
                  WHERE tp.child = p.parent
                    AND p.parent NOT IN (SELECT child
                                           FROM #parents)
        SELECT @nrows = @@ROWCOUNT
    END
    INSERT INTO #children(child, parent)
         SELECT child, parent
           FROM term_parents
          WHERE parent = @doc_id
    SELECT @nrows = @@ROWCOUNT
    WHILE @nrows > 0
    BEGIN
            INSERT INTO #children(child, parent)
        SELECT DISTINCT tp.child, tp.parent
                   FROM term_parents tp, #children c
                  WHERE tp.parent = c.child
                    AND c.child NOT IN (SELECT parent
                                          FROM #children)
        SELECT @nrows = @@ROWCOUNT
    END
    SELECT DISTINCT * FROM #parents
    UNION
    SELECT * FROM #children
    DROP TABLE #parents
    DROP TABLE #children

GO

CREATE TRIGGER dev_task_trigger ON dev_task
FOR UPDATE, INSERT
AS
    IF UPDATE(status)
    BEGIN
        UPDATE dev_task
          SET dev_task.status_date = GETDATE()
         FROM inserted, dev_task
        WHERE dev_task.id = inserted.id
    END

GO

/*
 * Another procedure required because of a bug in ADO, which complains
 * in the second query that the temporary table doesn't exist.  It's not
 * that ADO can't deal with temporary tables at all, but it gets very 
 * confused when placeholders and nested queries are combined.
 */
CREATE PROCEDURE cdr_newly_pub_trials
    @job_id INT
AS
    SELECT ppd1.doc_id,
           pp1.pub_subset
      INTO #prev_event
      FROM pub_proc_doc ppd1
      JOIN pub_proc pp1
        ON pp1.id = ppd1.pub_proc
      JOIN pub_proc_doc ppd2
        ON ppd2.doc_id = ppd1.doc_id
     WHERE ppd2.pub_proc = @job_id
       AND pp1.id = (SELECT MAX(pp3.id) as foobar
                       FROM pub_proc pp3
                       JOIN pub_proc_doc ppd3
                         ON ppd3.pub_proc = pp3.id
                        AND ppd3.doc_id = ppd1.doc_id
                       JOIN query_term ps
                         ON ps.doc_id = pp3.pub_system
                      WHERE pp3.id < ppd2.pub_proc
                        AND ps.path = '/PublishingSystem/SystemName'
                        AND ps.value = 'Primary'
                        AND pp3.pub_subset IN ('Export', 'Remove'))

 SELECT DISTINCT ppd.doc_id,
                 cat.value,
                 stat.value,
                 id.value
            FROM pub_proc_doc ppd
            JOIN query_term cat
              ON cat.doc_id = ppd.doc_id
            JOIN query_term cat_type
              ON cat_type.doc_id = cat.doc_id
             AND LEFT(cat_type.node_loc, 8) = LEFT(cat.node_loc, 8)
            JOIN query_term stat
              ON stat.doc_id = ppd.doc_id
             AND stat.path = '/InScopeProtocol/ProtocolAdminInfo'
                           + '/CurrentProtocolStatus'
            JOIN query_term id
              ON id.doc_id = ppd.doc_id
             AND id.path = '/InScopeProtocol/ProtocolIDs/PrimaryID/IDString'
           WHERE ppd.pub_proc = @job_id
             AND cat.path = '/InScopeProtocol/ProtocolDetail/StudyCategory'
                          + '/StudyCategoryName'
             AND cat_type.path = '/InScopeProtocol/ProtocolDetail'
                               + '/StudyCategory/StudyCategoryType'
             AND cat_type.value = 'Primary'
             AND ppd.doc_id NOT IN (SELECT doc_id
                                      FROM #prev_event
                                     WHERE pub_subset = 'Export')
        ORDER BY cat.value,
                 id.value,
                 stat.value
GO

/*
 * ADODB won't let us get to the temporary tables created for a single
 * connection, so we have to wrap up the queries for this report
 * in a stored procedure.
 */
CREATE PROCEDURE cdr_coop_group_report
    @docId INTEGER
AS
    /*
     * Get the main member organizations.
     */
    SELECT DISTINCT mm_d.id,
                    mm_d.title
               INTO #mm
               FROM document mm_d
               JOIN query_term mm
                 ON mm.doc_id = mm_d.id
              WHERE mm.path = '/Organization/OrganizationAffiliations' 
                            + '/MemberOfCooperativeGroups'
                            + '/MainMemberOf/CooperativeGroup/@cdr:ref'
                AND mm.int_val = @docId
    SELECT * FROM #mm ORDER BY title, id

    /*
     * Get the principal investigators for the main member organizations.
     */
    SELECT DISTINCT mm.id,
                    pi_d.id,
                    pi_d.title
               FROM document pi_d
               JOIN query_term pi
                 ON pi.doc_id = pi_d.id
               JOIN #mm mm
                 ON mm.id = pi.int_val
               JOIN query_term role
                 ON role.doc_id = pi_d.id
               JOIN query_term ca
                 ON ca.doc_id  = pi_d.id
              WHERE pi.path    = '/Person/PersonLocations'
                               + '/OtherPracticeLocation'
                               + '/OrganizationLocation/@cdr:ref'
                AND ca.path    = '/Person/PersonLocations'
                               + '/OtherPracticeLocation'
                               + '/ComplexAffiliation/Organization/@cdr:ref'
                AND role.path  = '/Person/PersonLocations'
                               + '/OtherPracticeLocation'
                               + '/ComplexAffiliation'
                               + '/RoleAtAffiliatedOrganization'
                AND role.value = 'Principal Investigator'
                AND ca.int_val = @docId
                AND LEFT(pi.node_loc, 8) = LEFT(ca.node_loc, 8)
                AND LEFT(ca.node_loc, 12) = LEFT(role.node_loc, 12)
           ORDER BY pi_d.title, pi_d.id

    /*
     * Get the affiliate members of the group.
     */
    SELECT DISTINCT mm.id AS mm_id,
                    am_d.id,
                    am_d.title
               INTO #am
               FROM document am_d
               JOIN query_term coop
                 ON coop.doc_id = am_d.id
                AND coop.path = '/Organization/OrganizationAffiliations'
                              + '/MemberOfCooperativeGroups/AffiliateMemberOf'
                              + '/CooperativeGroup/@cdr:ref'
    LEFT OUTER JOIN query_term am
                 ON am.doc_id = am_d.id
                AND am.path   = '/Organization/OrganizationAffiliations'
                              + '/MemberOfCooperativeGroups/AffiliateMemberOf'
                              + '/MainMember/@cdr:ref'
    LEFT OUTER JOIN #mm mm
                 ON mm.id = am.int_val
                AND LEFT(am.node_loc, 12) = LEFT(coop.node_loc, 12)
              WHERE coop.int_val = @docId
    SELECT * FROM #am ORDER BY title, id

    /*
     * Get the principal investigators for the affiliate member organizations.
     */
    SELECT DISTINCT am.mm_id,
                    am.id,
                    pi_d.id,
                    pi_d.title
               FROM document pi_d
               JOIN query_term pi
                 ON pi.doc_id = pi_d.id
               JOIN #am am
                 ON am.id = pi.int_val
               JOIN query_term role
                 ON role.doc_id = pi_d.id
               JOIN query_term ca
                 ON ca.doc_id = pi_d.id
              WHERE pi.path    = '/Person/PersonLocations'
                               + '/OtherPracticeLocation'
                               + '/OrganizationLocation/@cdr:ref'
                AND ca.path    = '/Person/PersonLocations'
                               + '/OtherPracticeLocation'
                               + '/ComplexAffiliation/Organization/@cdr:ref'
                AND role.path  = '/Person/PersonLocations'
                               + '/OtherPracticeLocation'
                               + '/ComplexAffiliation'
                               + '/RoleAtAffiliatedOrganization'
                AND role.value = 'Principal Investigator'
                AND ca.int_val = @docId
                AND LEFT(pi.node_loc, 8) = LEFT(ca.node_loc, 8)
                AND LEFT(ca.node_loc, 12) = LEFT(role.node_loc, 12)
           ORDER BY pi_d.title, pi_d.id

    /*
     * Clean up after ourselves.
     */
    DROP TABLE #am
    DROP TABLE #mm
GO
