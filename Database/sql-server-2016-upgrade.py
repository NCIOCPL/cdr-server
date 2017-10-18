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
cursor.execute("DROP PROCEDURE cdr_get_term_tree")
cursor.execute("""\
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
try:
    cursor.execute("DROP PROCEDURE cdr_get_tree_context")
except:
    pass
cursor.execute("""\
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
conn.commit()
