/*
 * $Id: procs.sql,v 1.7 2002-01-02 21:58:54 bkline Exp $
 *
 * Stored procedures for CDR.
 *
 * $Log: not supported by cvs2svn $
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

    SELECT DISTINCT #po.id as po, d.id, d.title
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
                AND role.value = 'Principal Investigator'
                AND lo.int_val = @lead_org

             SELECT po.id, po.title, #po.path, pi.id, pi.title
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
