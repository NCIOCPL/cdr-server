/*
 * First version of procedure to get Term document relationship.
 * Replaced by optimized version which uses a temporary table of integers
 * for the term_parents view.
 */
CREATE VIEW term_parents
         AS SELECT DISTINCT doc_id AS child,
                            CAST(SUBSTRING(value, 4, 10) AS INT) as parent
                       FROM query_term
                      WHERE path = '/Term/TermParent/@cdr:ref'
DROP PROCEDURE cdr_get_tree_context
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
