/*
 * $Id: procs.sql,v 1.2 2001-04-08 19:20:53 bkline Exp $
 *
 * Stored procedures for CDR.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2001/04/08 19:08:34  bkline
 * Initial revision
 */

/*
 * Drop old versions.
 */
IF EXISTS (SELECT *
             FROM sysobjects
            WHERE name = 'sp_find_address'
              AND type = 'P')
    DROP PROCEDURE cdr_get_term_tree
GO

/**
 * Finds all the parents and children of a Term document in the CDR.  Two
 * result sets are created.  The first contains document id pairs for
 * child-parent relationships.  The second result set contains the document id
 * and title for each id (child or parent) in the first result set.
 *
 *  @param  doc_id      primary key for Term document whose children and
 *                      parents are requested.
 */
CREATE PROCEDURE cdr_get_term_tree
    @doc_id INT
AS
    -- Used for loop control.
    DECLARE @nrows INT

    /*
     * Create a temporary table containing integers for all of the
     * child-parent relationships of CDR Term documents.  For all but the most
     * trivial requests the performance advantage of working with the integer
     * forms of the keys significantly outweighs the cost of creating the
     * temporary table from the term_parents view, which has the string
     * representation of the parent ID under its definition.
     */
    SELECT child, parent
      INTO #term_parents
      FROM term_parents
    CREATE INDEX idx_tpc ON #term_parents(child)
    CREATE INDEX idx_tpp ON #term_parents(parent)

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
           FROM #term_parents
          WHERE child = @doc_id

    /*
     * Continue gathering parents until we find no more.
     */
    SELECT @nrows = @@ROWCOUNT
    WHILE @nrows > 0
    BEGIN
            INSERT INTO #parents(child, parent)
        SELECT DISTINCT tp.child, tp.parent
                   FROM #term_parents tp, #parents p
                  WHERE tp.child = p.parent
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
           FROM #term_parents
          WHERE parent = @doc_id

    /*
     * Continue gathering children until we find no more.
     */
    SELECT @nrows = @@ROWCOUNT
    WHILE @nrows > 0
    BEGIN
            INSERT INTO #children(child, parent)
        SELECT DISTINCT tp.child, tp.parent
                   FROM #term_parents tp, #children c
                  WHERE tp.parent = c.child
                    AND c.child NOT IN (SELECT parent
                                          FROM #children)
        SELECT @nrows = @@ROWCOUNT
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
    DROP TABLE #term_parents
GO
