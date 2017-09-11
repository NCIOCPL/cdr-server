-- Script to create tables used by the terminology audio review program
-- BZIssue::5020

/*
 * One row for each zipfile containing a spreadsheet plus mp3 files.
 *
 *          id - Unique id.
 *    filename - Name of the zipfile, without path.
 *    filedate - Datetime listed on the zipfile.
 *    complete - 'Y' = all mp3's reviewed, 'N' = more to go.
 */
CREATE TABLE term_audio_zipfile (
            id  INTEGER         IDENTITY PRIMARY KEY,
      filename  VARCHAR(128)    NOT NULL UNIQUE,
      filedate  DATETIME        NOT NULL,
      complete  CHAR            DEFAULT 'N'
)

GO

GRANT SELECT, UPDATE, INSERT ON term_audio_zipfile TO CdrGuest
GO

/*
 * One row for each mp3 file in a zipfile.
 *
 * Most columns are taken from the spreadsheet.  Others are added
 * for processing.
 *
 *              id - Unique id.
 *      zipfile_id - ID of row in term_audio_zipfile for mp3 source file.
 *   review_status - 'A'pproved, 'R'ejected or 'U'nreviewed.
 *          cdr_id - CDR ID of the term to be pronounced.
 *       term_name - The term itself.
 *        language - 'en'glish or 'es'panol.
 *   pronunciation - Phonetic spelling of English terms, NULL for Spanish.
 *        mp3_name - MP3 filename in zipfile, includes path, if any.
 *     reader_note - Note by person who recorded the mp3.
 *   reviewer_note - Note by reviewer, usually only found in 'R'ejects.
 *     reviewer_id - Link to usr table for user who did the review.
 *     review_date - Datetime of last action on this mp3 file, if any.
 */
CREATE TABLE term_audio_mp3 (
            id  INTEGER         IDENTITY PRIMARY KEY,
    zipfile_id  INTEGER         NOT NULL REFERENCES term_audio_zipfile(id),
 review_status  CHAR            DEFAULT 'U',
        cdr_id  INTEGER         NOT NULL,
     term_name  VARCHAR(256)    NOT NULL,
      language  VARCHAR(10)     NOT NULL,
 pronunciation  VARCHAR(256)    NULL,
      mp3_name  VARCHAR(256)    NULL,
   reader_note  VARCHAR(2048)   NULL,
 reviewer_note  VARCHAR(2048)   NULL,
   reviewer_id  INTEGER         NULL REFERENCES usr(id),
   review_date  DATETIME        NULL
)
GO
GRANT SELECT, UPDATE, INSERT ON term_audio_mp3 TO CdrGuest
GO

/*
 * One row, the most recent one, for each actual term.
 *
 * Due to character set problems, the terms don't always exactly match,
 * but every combination of cdr_id + language should be unique.
 *
 *  max_date - the date of the last review for each unique term.
 *
 * Inner joining on this table will ensure that only the last review of a
 * term is examined.
 */
CREATE VIEW lastmp3 AS
     SELECT MAX(review_date) AS max_date, cdr_id, language 
       FROM term_audio_mp3
   GROUP BY cdr_id, language
GO
GRANT SELECT ON lastmp3 TO CdrGuest
GO
