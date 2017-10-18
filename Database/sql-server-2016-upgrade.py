#!/usr/bin/env python

import cdrdb2 as cdrdb

conn = cdrdb.connect()
cursor = conn.cursor()
try:
    cursor.execute("DROP VIEW term_children")
except:
    pass
cursor.execute("""\
CREATE VIEW term_children
         AS SELECT DISTINCT q1.int_val AS parent,
                            q1.doc_id  AS child,
                            COUNT(DISTINCT q2.doc_id) as num_grandchildren,
                            d.title
                       FROM document d
                       JOIN query_term q1
                         ON q1.doc_id = d.id
            LEFT OUTER JOIN query_term q2
                         ON q1.doc_id = q2.int_val
                      WHERE q1.path = '/Term/TermParent/@cdr:ref'
                        AND q2.path = '/Term/TermParent/@cdr:ref'
                   GROUP BY q1.int_val, q1.doc_id, d.title
""")
cursor.execute("GRANT SELECT ON term_children TO CdrGuest")
cursor.execute("GRANT SELECT ON term_children TO CdrPublishing")
try:
    cursor.execute("DROP TRIGGER dev_task_trigger")
except:
    pass
try:
    cursor.execute("DROP PROCEDURE cdr_get_term_tree")
except:
    pass
cursor.execute("""\
CREATE PROCEDURE cdr_get_term_tree
    @doc_id INT,
    @depth  INT
AS

    -- We don't know why but proc fails without this
    -- when invoked from a scripting language.
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
""")
cursor.execute("GRANT EXECUTE ON cdr_get_term_tree TO CdrGuest")

try:
    cursor.execute("DROP PROCEDURE get_participating_orgs")
except:
    pass
cursor.execute("""\
CREATE PROCEDURE get_participating_orgs
    @lead_org INTEGER
AS

    -- Do this or fail!
    SET NOCOUNT ON;

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
""")
cursor.execute("GRANT EXECUTE ON get_participating_orgs TO CdrGuest")

try:
    cursor.execute("DROP PROCEDURE cdr_get_tree_context")
except:
    pass
cursor.execute("""\
CREATE PROCEDURE cdr_get_tree_context
    @doc_id INT
AS
    -- Do this or fail!
    SET NOCOUNT ON;

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
                   FROM term_parents tp
                   JOIN #parents p
                     ON tp.child = p.parent
                  WHERE p.parent NOT IN (SELECT child
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
                   FROM term_parents tp
                   JOIN #children c
                     ON tp.parent = c.child
                  WHERE c.child NOT IN (SELECT parent
                                          FROM #children)
        SELECT @nrows = @@ROWCOUNT
    END
    SELECT DISTINCT * FROM #parents
    UNION
    SELECT * FROM #children
    DROP TABLE #parents
    DROP TABLE #children
""")
cursor.execute("GRANT EXECUTE ON cdr_get_tree_context TO CdrGuest")

try:
    cursor.execute("DROP VIEW doc_header")
except:
    pass
cursor.execute("""\
CREATE VIEW doc_header
AS
  SELECT d.id AS DocId,
         t.name AS DocType,
         d.title AS DocTitle
    FROM document d
    JOIN doc_type t
      ON d.doc_type = t.id
""")
cursor.execute("GRANT SELECT ON doc_header TO CdrGuest")
cursor.execute("GRANT SELECT ON doc_header TO CdrPublishing")
try:
    cursor.execute("DROP VIEW orphan_terms")
except:
    pass
cursor.execute("""\
CREATE VIEW orphan_terms
AS
    SELECT DISTINCT d.title
               FROM document d
               JOIN query_term q
                 ON d.id = q.doc_id
              WHERE q.path = '/Term/TermPrimaryType'
                AND q.value <> 'glossary term'
                AND NOT EXISTS (SELECT *
                                  FROM query_term q2
                                 WHERE q2.doc_id = d.id
                                   AND q2.path = '/Term/TermParent/@cdr:ref')
""")
cursor.execute("GRANT SELECT ON orphan_terms TO CdrGuest")
cursor.execute("GRANT SELECT ON orphan_terms TO CdrPublishing")
try:
    cursor.execute("DROP VIEW published_doc")
except:
    pass
cursor.execute("""\
CREATE VIEW published_doc
         AS SELECT pub_proc_doc.*
              FROM pub_proc_doc
              JOIN pub_event
                ON pub_event.id = pub_proc_doc.pub_proc
             WHERE pub_event.status = 'Success'
               AND (pub_proc_doc.failure IS NULL
                OR pub_proc_doc.failure <> 'Y')
""")
cursor.execute("GRANT SELECT ON published_doc TO CdrGuest")
cursor.execute("GRANT SELECT ON published_doc TO CdrPublishing")
try:
    cursor.execute("DROP VIEW TermsWithParents")
except:
    pass
cursor.execute("""\
CREATE VIEW TermsWithParents
AS
    SELECT d.id, d.xml
      FROM document d
      JOIN doc_type t
        ON d.doc_type = t.id
     WHERE t.name = 'Term'
       AND d.xml LIKE '%<TermParent%'
""")
cursor.execute("GRANT SELECT ON TermsWithParents TO CdrGuest")
cursor.execute("GRANT SELECT ON TermsWithParents TO CdrPublishing")
try:
    cursor.execute("DROP VIEW term_kids")
except:
    pass
cursor.execute("""\
CREATE VIEW term_kids
         AS SELECT DISTINCT q.int_val AS parent,
                            q.doc_id  AS child,
                            d.title
                       FROM query_term q
                       JOIN document d
                         ON d.id = q.doc_id
                      WHERE q.path =
                            '/Term/TermRelationship/ParentTerm/TermId/@cdr:ref'
""")
cursor.execute("GRANT SELECT ON term_kids TO CdrGuest")
cursor.execute("GRANT SELECT ON term_kids TO CdrPublishing")
try:
    cursor.execute("DROP PROCEDURE cdr_newly_pub_trials")
except:
    pass

try:
    cursor.execute("DROP PROCEDURE cdr_coop_group_report")
except:
    pass
cursor.execute("""\
CREATE PROCEDURE cdr_coop_group_report
    @docId INTEGER
AS
    /*
     * Get the main member organizations.
     */

    -- Do this or fail!
    SET NOCOUNT ON;

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
""")
cursor.execute("GRANT EXECUTE ON cdr_coop_group_report TO CdrGuest")

try:
    cursor.execute("DROP PROCEDURE cdr_get_count_of_links_to_persons")
except:
    pass
cursor.execute("""\
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
""")
cursor.execute("GRANT EXECUTE ON cdr_get_count_of_links_to_persons TO CdrGuest")

try:
    cursor.execute("DROP PROCEDURE find_linked_docs")
except:
    pass
cursor.execute("""\
CREATE PROC find_linked_docs
    @doc_id INTEGER
AS
    -- Do this or fail!
    SET NOCOUNT ON;

    -- Local variables.
    DECLARE @nrows INTEGER

    -- Create a temporary table for the linked documents.
    CREATE TABLE #linked_docs (id INTEGER, doc_type VARCHAR(32))

    -- Seed the table with the caller's document ID.
    INSERT INTO #linked_docs
         SELECT @doc_id,
                t.name
           FROM document d
           JOIN doc_type t
             ON t.id = d.doc_type
          WHERE d.id = @doc_id

    -- Keep inserting until we exhaust the links.
    -- Don't get citations.
    SELECT @nrows = @@ROWCOUNT
    WHILE @nrows > 0
    BEGIN
        INSERT INTO #linked_docs
        SELECT DISTINCT d.id, t.name
                   FROM document d
                   JOIN link_net n
                     ON n.target_doc = d.id
                   JOIN doc_type t
                     ON t.id = d.doc_type
                   JOIN #linked_docs ld
                     ON ld.id = n.source_doc
                  WHERE d.id NOT IN (SELECT id FROM #linked_docs)
                    AND t.name <> 'Citation'
        SELECT @nrows = @@ROWCOUNT
    END

    -- Uncomment this next row if you want to drop the original document
    -- DELETE FROM #linked_docs WHERE id = @doc_id

    -- Give the caller the results.
    SELECT DISTINCT id, doc_type FROM #linked_docs

    -- Clean up after ourselves.
    DROP TABLE #linked_docs
""")
cursor.execute("GRANT EXECUTE ON find_linked_docs TO CdrGuest")

try:
    cursor.execute("DROP PROCEDURE get_prot_person_connections")
except:
    pass
cursor.execute("""\
CREATE PROC get_prot_person_connections
    @doc_id INTEGER
AS
    -- Do this or fail!
    SET NOCOUNT ON;

    -- Local variables.
    DECLARE @active_trials INTEGER
    DECLARE @closed_trials INTEGER

    -- Create a temporary table for person as non-PUP lead org person.
    CREATE TABLE #lead_org_person (prot_id INTEGER,
                                   lead_org_status VARCHAR(32))

    INSERT INTO #lead_org_person
SELECT DISTINCT lead_org_stat.doc_id prot_id,
                lead_org_stat.value lead_org_status
           FROM query_term lead_org_stat
           JOIN query_term person
             ON person.doc_id = lead_org_stat.doc_id
            AND LEFT(person.node_loc, 8) = LEFT(lead_org_stat.node_loc, 8)
           JOIN query_term person_role
             ON person.doc_id = person_role.doc_id
            AND LEFT(person.node_loc, 12) = LEFT(person_role.node_loc, 12)
          WHERE lead_org_stat.path = '/InScopeProtocol/ProtocolAdminInfo'
                                   + '/ProtocolLeadOrg/LeadOrgProtocolStatuses'
                                   + '/CurrentOrgStatus/StatusName'
            AND person.path        = '/InScopeProtocol/ProtocolAdminInfo'
                                   + '/ProtocolLeadOrg/LeadOrgPersonnel'
                                   + '/Person/@cdr:ref'
            AND person_role.path   = '/InScopeProtocol/ProtocolAdminInfo'
                                   + '/ProtocolLeadOrg/LeadOrgPersonnel'
                                   + '/PersonRole'
            AND person_role.value <> 'Update Person'
            AND EXISTS (SELECT *
                          FROM primary_pub_doc
                         WHERE doc_id = lead_org_stat.doc_id)
            AND person.int_val = @doc_id

    -- Create a temporary table for person at private practice site.
    CREATE TABLE #private_practice_person (prot_id INTEGER)

    INSERT INTO #private_practice_person (prot_id)
SELECT DISTINCT lead_org_stat.doc_id prot_id
           FROM query_term lead_org_stat
           JOIN query_term person
             ON person.doc_id = lead_org_stat.doc_id
            AND LEFT(person.node_loc, 8) = LEFT(lead_org_stat.node_loc, 8)
           JOIN query_term person_status
             ON person_status.doc_id = person.doc_id
            AND LEFT(person_status.node_loc, 16) = LEFT(person.node_loc, 16)
          WHERE lead_org_stat.path = '/InScopeProtocol/ProtocolAdminInfo'
                                   + '/ProtocolLeadOrg/LeadOrgProtocolStatuses'
                                   + '/CurrentOrgStatus/StatusName'
            AND person.path        = '/InScopeProtocol/ProtocolAdminInfo'
                                   + '/ProtocolLeadOrg/ProtocolSites'
                                   + '/PrivatePracticeSite'
                                   + '/PrivatePracticeSiteID/@cdr:ref'
            AND person_status.path = '/InScopeProtocol/ProtocolAdminInfo'
                                   + '/ProtocolLeadOrg/ProtocolSites'
                                   + '/PrivatePracticeSite'
                                   + '/PrivatePracticeSiteStatus'
            AND lead_org_stat.value = 'Active'
            AND person_status.value = 'Active'
            AND EXISTS (SELECT *
                          FROM primary_pub_doc
                         WHERE doc_id = lead_org_stat.doc_id)
            AND person.int_val = @doc_id

    -- Create temporary table for connections to org sites.
    CREATE TABLE #org_site_person (prot_id INTEGER)

    INSERT INTO #org_site_person
SELECT DISTINCT lead_org_stat.doc_id prot_id
           FROM query_term lead_org_stat
           JOIN query_term person
             ON person.doc_id = lead_org_stat.doc_id
            AND LEFT(person.node_loc, 8) = LEFT(lead_org_stat.node_loc, 8)
           JOIN query_term site_status
             ON site_status.doc_id = person.doc_id
            AND LEFT(site_status.node_loc, 16) = LEFT(person.node_loc, 16)
          WHERE lead_org_stat.path = '/InScopeProtocol/ProtocolAdminInfo'
                                   + '/ProtocolLeadOrg/LeadOrgProtocolStatuses'
                                   + '/CurrentOrgStatus/StatusName'
            AND person.path        = '/InScopeProtocol/ProtocolAdminInfo'
                                   + '/ProtocolLeadOrg/ProtocolSites/OrgSite'
                                   + '/OrgSiteContact/SpecificPerson'
                                   + '/Person/@cdr:ref'
            AND site_status.path   = '/InScopeProtocol/ProtocolAdminInfo'
                                   + '/ProtocolLeadOrg/ProtocolSites/OrgSite'
                                   + '/OrgSiteStatus'
            AND lead_org_stat.value = 'Active'
            AND site_status.value = 'Active'
            AND EXISTS (SELECT *
                          FROM primary_pub_doc
                         WHERE doc_id = lead_org_stat.doc_id)
            AND person.int_val = @doc_id

    -- Collect the count of active protocols from all three temp tables.
    SELECT @active_trials = (SELECT COUNT(*) FROM (
        SELECT prot_id
          FROM #lead_org_person
         WHERE lead_org_status IN ('Approved-not yet active', 'Active')
         UNION
        SELECT prot_id
          FROM #private_practice_person
         UNION
        SELECT prot_id
          FROM #org_site_person) AS inner_select)

    -- Get the count of closed trials from the first table.
    SELECT @closed_trials = (
    SELECT COUNT(*)
      FROM #lead_org_person
     WHERE lead_org_status IN ('Closed',
                               'Completed',
                               'Temporarily Closed'))

    -- Return the counts.
    SELECT @active_trials, @closed_trials

    -- Clean up after ourselves.
    DROP TABLE #lead_org_person
    DROP TABLE #private_practice_person
    DROP TABLE #org_site_person
""")
cursor.execute("GRANT EXECUTE ON get_prot_person_connections TO CdrGuest")

try:
    cursor.execute("DROP PROCEDURE select_changed_non_active_protocols")
except:
    pass
cursor.execute("""\
CREATE PROC select_changed_non_active_protocols

AS
    -- Do this or fail!
    SET NOCOUNT ON;

    -- Create temporary table containing docId + version num of
    --   all currently published docs sent to cancer.gov
    SELECT doc_id AS id, MAX(doc_version) AS num
      INTO #pubver_prot
      FROM pub_proc_doc
     WHERE failure <> 'Y'
        OR failure IS NULL
     GROUP BY doc_id

    -- Create temporary table containing latest non-active protocol
    --   publishable version ids and version numbers
    SELECT v.id AS vid, max(v.num) AS vnum
      INTO #latestver_prot
      FROM doc_version v
      JOIN all_docs d
        ON v.id = d.id
      JOIN query_term_pub q
        ON q.doc_id = d.id
     WHERE q.path =
           '/InScopeProtocol/ProtocolAdminInfo/CurrentProtocolStatus'
       AND q.value IN ('Closed', 'Completed', 'Temporarily Closed')
       AND d.active_status = 'A'
       AND v.publishable = 'Y'
       AND v.val_status = 'V'
     GROUP BY v.id

    -- Select those non-active protocol ids and versions for which
    --   there is no publishable version at all, or only an older one
    SELECT #latestver_prot.vid, #latestver_prot.vnum
      FROM #latestver_prot
      LEFT OUTER JOIN #pubver_prot
        ON #latestver_prot.vid = #pubver_prot.id
     WHERE #pubver_prot.id IS NULL
        OR #latestver_prot.vnum > #pubver_prot.num
""")
cursor.execute("""\
GRANT EXECUTE
           ON select_changed_non_active_protocols
           TO CdrGuest""")

conn.commit()
