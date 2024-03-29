/*
 * Stored procedures for CDR.
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
            WHERE name = 'cdr_get_count_of_links_to_persons'
              AND type = 'P')
    DROP PROCEDURE cdr_get_count_of_links_to_persons
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

    -- Do this or fail!
    SET NOCOUNT ON;

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
                   FROM term_kids tk
                   JOIN #parents p
                     ON tk.child = p.parent
                  WHERE p.parent NOT IN (SELECT child
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
                   FROM term_kids tk
                   JOIN #children c
                     ON tk.parent = c.child
                  WHERE c.child NOT IN (SELECT parent
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
 * Pull out count of protocol and summary documents linking to a
 * specified Person document (used by the Person QC report).
 *
 * @todo Since the first of the two queries below will now always
 *       find nothing, we should rewrite the QC report to invoke
 *       the second query directly and dispense with this SP.
 */
CREATE PROCEDURE cdr_get_count_of_links_to_persons
    @docId INTEGER
AS

    -- Speeds things up.
    SET NOCOUNT ON;

    SELECT COUNT(person.doc_id), status.value
      FROM query_term person
      JOIN query_term status
        ON status.doc_id = person.doc_id
     WHERE person.path IN ('/InScopeProtocol/ProtocolAdminInfo' +
                           '/ProtocolLeadOrg/LeadOrgPersonnel/Person/@cdr:ref',
                           '/InScopeProtocol/ProtocolAdminInfo' +
                           '/ProtocolLeadOrg/ProtocolSites/OrgSite' +
                           '/OrgSiteContact/SpecificPerson/Person/@cdr:ref',
                           '/InScopeProtocol/ProtocolAdminInfo' +
                           '/ProtocolLeadOrg/ProtocolSites' +
                           '/PrivatePracticeSite' +
                           '/PrivatePracticeSiteID/@cdr:ref',
                           '/InScopeProtocol/ProtocolAdminInfo' +
                           '/ExternalSites/ExternalSite' +
                           '/ExternalSitePI/ExternalSitePIID/@cdr:ref')
       AND status.path   = '/InScopeProtocol/ProtocolAdminInfo'
                         + '/CurrentProtocolStatus'
       AND person.int_val = @docId
  GROUP BY status.value

    SELECT COUNT(person.doc_id), audience.value
      FROM query_term person
      JOIN query_term audience
        ON audience.doc_id = person.doc_id
     WHERE person.path     = '/Summary/SummaryMetaData'
                           + '/PDQBoard/BoardMember/@cdr:ref'
       AND audience.path   = '/Summary/SummaryMetaData/SummaryAudience'
       AND person.int_val  = @docId
  GROUP BY audience.value
GO

/*
 * We found out the hard way that this is the only way to keep programmers
 * from manipulating the SQL tables for CDR documents directly.
 */
CREATE TRIGGER NoDelVersion ON all_doc_versions
FOR DELETE
AS
    RAISERROR ('CDR Versions are permanent.', 16, 1)
    ROLLBACK TRANSACTION
GO

/*
 * Same principle as for the all_doc_versions table.
 */
CREATE TRIGGER CdrDelDoc ON all_docs
FOR DELETE
AS
    RAISERROR ('Use CdrDelDoc from the CDR client/server API instead.', 16, 1)
    ROLLBACK TRANSACTION
GO

/*
 * Make sure a CDR document is not marked as 'D'eleted if there's a row
 * in the external_map table which maps to it.
 */
CREATE TRIGGER cdr_mod_doc ON all_docs
FOR UPDATE
AS
    IF UPDATE(active_status)
    BEGIN
        IF EXISTS (SELECT i.id
                     FROM cdr..external_map m
                     JOIN inserted i
                       ON m.doc_id = i.id
                    WHERE i.active_status = 'D')
        BEGIN
            RAISERROR('Attempt to delete document in external_map table', 16, 1)
            ROLLBACK TRANSACTION
        END
    END
GO

/*
 * Prevent the guest session from being logged out.
 */
CREATE TRIGGER guest_protection ON session
FOR UPDATE
AS
    IF UPDATE(ended)
    BEGIN
        IF EXISTS (SELECT *
                     FROM inserted
                    WHERE name = 'guest'
                      AND ended IS NOT NULL)
        BEGIN
            RAISERROR ('Attempt to log out guest account', 11, 1)
            ROLLBACK TRANSACTION
        END
    END
GO
