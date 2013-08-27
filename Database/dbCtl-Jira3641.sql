/**
 * This table will be replaced with a new version below.
 */
DROP TABLE ctl
GO

/**
 * This table was never used and is not used in the new version of ctl.
 */
DROP TABLE ctl_grp
GO 

/**
 * Named and grouped values used to control various run-time settings.
 *
 * Software in the CdrServer enforces a constraint that only one grp+name
 * combination is allowed to be active (i.e., inactivated is null) at a time.
 * If there is more than one, the software will throw an exception.  An
 * attempt to add one via the CdrServer software will inactivate any existing
 * active row with the same grp+name.
 *
 *           id  unique row ID.
 *          grp  identifies group this name=value belongs to, e.g. "dbconnect"
 *         name  mnemonic label for value; e.g., "timeout"
 *          val  value associated with this name; e.g., "3000"
 *      comment  optional text description of the use of this value,
 *                e.g., "Database connection timeout, in milliseconds"
 *      created  Datetime this row was created
 *  inactivated  Datetime row made inactive, not used on controlling ops
 *
 *  BZIssue::OCECDR-3641
 *
 */
CREATE TABLE ctl
         (id INTEGER         IDENTITY PRIMARY KEY,
         grp VARCHAR(255)    NOT NULL,
        name VARCHAR(255)    NOT NULL,
         val NVARCHAR(255)   NOT NULL,
     comment VARCHAR(255)    NULL,
     created DATETIME        NOT NULL,
 inactivated DATETIME        NULL)
GO
