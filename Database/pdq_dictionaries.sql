/**
 * Create the database to provide support for the drug dictionary.
 *
 * This support was formerly supplied by GateKeeper, which will
 * soon be retired. The functionality will be taken over by the
 * new drug dictionary API and React front end, but the GateKeeper
 * retirement will happen before those are ready to be released,
 * so the CDE will use the tables created here (and populated by
 * a scheduled CDR job) until that happens.
 */
USE master
GO

CREATE DATABASE pdq_dictionaries
GO

USE pdq_dictionaries
GO

/* Grant access to the following accounts. */
EXEC sp_grantdbaccess 'CdrSqlAccount'
EXEC sp_grantdbaccess 'CdrGuest'
EXEC sp_grantdbaccess 'CdrPublishing'
EXEC sp_grantdbaccess 'websiteuser'
GO

/* Create the main tables. */
CREATE TABLE Dictionary (
    TermID        INTEGER NOT NULL,
    TermName      NVARCHAR(1000) NOT NULL,
    Dictionary    NVARCHAR(10) NOT NULL,
    Language      NVARCHAR(20) NOT NULL,
    Audience      NVARCHAR(25) NOT NULL,
    ApiVers       NVARCHAR(10) NOT NULL,
    Object        NVARCHAR(MAX) NOT NULL
)
GO

CREATE TABLE DictionaryTermAlias (
    TermID        INTEGER NOT NULL,
    OtherName     NVARCHAR(1000) NOT NULL,
    OtherNameType NVARCHAR(30) NOT NULL,
    Language      NVARCHAR(20) NOT NULL,
	othernamelen  INTEGER NULL
)
GO

/* Create the working tables populated by the nightly job. */
CREATE TABLE Dictionary_Work (
    TermID        INTEGER NOT NULL,
    TermName      NVARCHAR(1000) NOT NULL,
    Dictionary    NVARCHAR(10) NOT NULL,
    Language      NVARCHAR(20) NOT NULL,
    Audience      NVARCHAR(25) NOT NULL,
    ApiVers       NVARCHAR(10) NOT NULL,
    Object        NVARCHAR(MAX) NOT NULL
)
GO

CREATE TABLE DictionaryTermAlias_Work (
    TermID        INTEGER NOT NULL,
    OtherName     NVARCHAR(1000) NOT NULL,
    OtherNameType NVARCHAR(30) NOT NULL,
    Language      NVARCHAR(20) NOT NULL,
	othernamelen  INTEGER NULL
)
GO

/* Add the necessary indexes. */
USE pdq_dictionaries
GO

/****** Object:  Index [CI_dictionary]    Script Date: 6/26/2020 3:43:01 PM ******/
CREATE CLUSTERED INDEX [CI_dictionary] ON [dbo].[Dictionary]
(
	[TermID] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, SORT_IN_TEMPDB = OFF, DROP_EXISTING = OFF, ONLINE = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
GO

USE pdq_dictionaries
GO

SET ANSI_PADDING ON
GO

/****** Object:  Index [NC_dictionary_termname]    Script Date: 6/26/2020 3:44:01 PM ******/
CREATE NONCLUSTERED INDEX [NC_dictionary_termname] ON [dbo].[Dictionary]
(
	[TermName] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, SORT_IN_TEMPDB = OFF, DROP_EXISTING = OFF, ONLINE = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
GO

USE pdq_dictionaries
GO

/****** Object:  Index [CI_dictionaryTermalias]    Script Date: 6/26/2020 3:48:58 PM ******/
CREATE CLUSTERED INDEX [CI_dictionaryTermalias] ON [dbo].[DictionaryTermAlias]
(
	[TermID] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, SORT_IN_TEMPDB = OFF, DROP_EXISTING = OFF, ONLINE = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
GO

USE pdq_dictionaries
GO

SET ANSI_PADDING ON
GO

/****** Object:  Index [NC_dictionarytermalias_othername]    Script Date: 6/26/2020 3:49:36 PM ******/
CREATE NONCLUSTERED INDEX [NC_dictionarytermalias_othername] ON [dbo].[DictionaryTermAlias]
(
	[Othername] ASC
)
WHERE ([othernamelen]<(450))
WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, SORT_IN_TEMPDB = OFF, DROP_EXISTING = OFF, ONLINE = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
GO

/* Grant the required permissions. */
GRANT ALTER, INSERT, DELETE, UPDATE, SELECT ON Dictionary TO CdrSqlAccount WITH GRANT OPTION
GRANT ALTER, INSERT, DELETE, UPDATE, SELECT ON DictionaryTermAlias TO CdrSqlAccount WITH GRANT OPTION
GRANT ALTER, INSERT, DELETE, UPDATE, SELECT ON Dictionary_Work TO CdrSqlAccount WITH GRANT OPTION
GRANT ALTER, INSERT, DELETE, UPDATE, SELECT ON DictionaryTermAlias_Work TO CdrSqlAccount WITH GRANT OPTION
GRANT CREATE DEFAULT TO CdrSqlAccount
GRANT CREATE FUNCTION TO CdrSqlAccount
GRANT CREATE PROCEDURE TO CdrSqlAccount
GRANT CREATE RULE TO CdrSqlAccount
GRANT CREATE TABLE TO CdrSqlAccount
GRANT CREATE TYPE TO CdrSqlAccount
GRANT CREATE VIEW TO CdrSqlAccount
GRANT SELECT ON Dictionary TO CdrGuest
GRANT SELECT ON Dictionary TO CdrPublishing
GRANT SELECT ON DictionaryTermAlias TO CdrGuest
GRANT SELECT ON DictionaryTermAlias TO CdrPublishing
GO

/* Create the stored procedures invoked by the front end.
 * These have been cloned unmodified from CDRLiveGK.
 */
USE pdq_dictionaries
GO

/****** Object:  UserDefinedTableType [dbo].[udt_DictionaryAliasTypeFilter]    Script Date: 6/23/2020 2:41:53 PM ******/
CREATE TYPE [dbo].[udt_DictionaryAliasTypeFilter] AS TABLE(
	[NameType] [nvarchar](30) NULL
)
GO

USE pdq_dictionaries
GO

/****** Object:  UserDefinedTableType [dbo].[udt_DictionaryTerm]    Script Date: 6/23/2020 2:42:38 PM ******/
CREATE TYPE [dbo].[udt_DictionaryTerm] AS TABLE(
	[cdrid] [int] NULL,
	[dictionaryType] [varchar](20) NULL,
	[language] [varchar](40) NULL,
	[audience] [varchar](50) NULL
)
GO



USE pdq_dictionaries
GO

/****** Object:  StoredProcedure [dbo].[usp_GetDictionaryTerm]    Script Date: 6/23/2020 2:31:35 PM ******/
SET ANSI_NULLS ON
GO

SET QUOTED_IDENTIFIER ON
GO


-- Retrieves a specific term

CREATE PROCEDURE [dbo].[usp_GetDictionaryTerm]
	@TermID int,	-- Identifier for the term (not just a row)
	@Dictionary nvarchar(10),
	@Language nvarchar(20),
	@Audience nvarchar(25),
	@ApiVers nvarchar(10) -- What version of the API (there may be multiple).

AS
BEGIN
	-- SET NOCOUNT ON added to prevent extra result sets from
	-- interfering with SELECT statements.
	SET NOCOUNT ON;

	if @dictionary = 'drug'
	BEGIN
		select top 1 TermID, TermName, Dictionary, Language, Audience, ApiVers, object
				from dbo.dictionary
				where TermID = @TermID
				  and Dictionary = @Dictionary
				  and Language = @Language
				  and ApiVers = @ApiVers
		order by audience

		return
	END


	declare @i int

	declare @termSort TABLE(
     Dictionary nvarchar(10) NOT NULL,
     Audience nvarchar(25) NOT NULL,
     Cycle int NOT NULL
	)


	if @audience = 'Patient'
		 INSERT INTO @termSort VALUES
		  ('Term', 'Patient', 1)
		 ,('Term', 'HealthProfessional', 2)
		 ,('NotSet', 'Patient', 3)
		 ,('NotSet', 'HealthProfessional', 4)
		 ,('Genetic', 'Patient', 5)
		 ,('Genetic', 'HealthProfessional', 6)
	ELSE
		if @audience = 'HealthProfessional' and @dictionary <> 'Genetic'
			INSERT INTO @termSort VALUES
				 ('NotSet', 'HealthProfessional', 1)
				,('NotSet', 'Patient', 2)
				,('Genetic', 'HealthProfessional', 3)
				,('Genetic', 'Patient', 4)
				,('Term', 'HealthProfessional', 5)
				,('Term', 'Patient', 6)
			ELSE
				if @audience = 'HealthProfessional' and @dictionary = 'Genetic'
				INSERT INTO @termSort VALUES
					 ('Genetic', 'HealthProfessional', 1)
					,('Genetic', 'Patient', 2)
					,('Term', 'HealthProfessional', 3)
					,('Term', 'Patient', 4)
					,('NotSet', 'HealthProfessional', 5)
					,('NotSet', 'Patient', 6)




 	IF exists
		(select TermID, TermName, Dictionary, Language, Audience, ApiVers, object
		from dbo.Dictionary
		where TermID = @TermID
		  and Dictionary = @Dictionary
		  and Language = @Language
		  and Audience = @Audience
		  and ApiVers = @ApiVers)
			  select TermID, TermName, Dictionary, Language, Audience, ApiVers, object
					from Dictionary
					where TermID = @TermID
					  and Dictionary = @Dictionary
					  and Language = @Language
					  and Audience = @Audience
					  and ApiVers = @ApiVers

	  ELSE
			BEGIN
				select @i = t.Cycle from @termSort t where t.dictionary = @dictionary and t.Audience = @Audience

				if exists
				(
				select TermID, TermName, d.Dictionary, Language, d.Audience, ApiVers, object
				from Dictionary d inner join @termSort t on d.Dictionary = t.Dictionary and d.Audience = t.Audience
					where TermID = @TermID
						and Language = @Language
						and ApiVers = @ApiVers
						and t.cycle >= @i
					 )

					 select top 1 TermID, TermName, d.Dictionary, Language, d.Audience, ApiVers, object
							from Dictionary d inner join @termSort t on d.Dictionary = t.Dictionary and d.Audience = t.Audience
							where TermID = @TermID
								and Language = @Language
								and ApiVers = @ApiVers
								and t.cycle > =@i
							 order by t.cycle
				ELSE
						select top 1 TermID, TermName, d.Dictionary, Language, d.Audience, ApiVers, object
							from Dictionary d inner join @termSort t on d.Dictionary = t.Dictionary and d.Audience = t.Audience
							where TermID = @TermID
								and Language = @Language
								and ApiVers = @ApiVers
								order by cycle


			END

END
GO

USE pdq_dictionaries
GO

/****** Object:  StoredProcedure [dbo].[usp_GetDictionaryTermForAudience]    Script Date: 6/23/2020 2:34:04 PM ******/
SET ANSI_NULLS ON
GO

SET QUOTED_IDENTIFIER ON
GO



create procedure [dbo].[usp_GetDictionaryTermForAudience] (
				@TermID int,     -- Identifier for the term (not just a row)
                @Language nvarchar(20),
                @PreferredAudience nvarchar(25),
                @ApiVers nvarchar(10) -- What version of the API (there may be multiple).
)

AS
BEGIN

	SET NOCOUNT ON;

	if exists (select *
					from dbo.Dictionary
					where TermID = @TermID
					  and Language = @Language
					  and Audience = @PreferredAudience
					  and ApiVers = @ApiVers
					  )
			select TermID, TermName, Dictionary, Language, Audience, ApiVers, object
			from dbo.Dictionary
			where TermID = @TermID
			  and Language = @Language
			  and Audience = @PreferredAudience
			  and ApiVers = @ApiVers
		ELSE
			select TermID, TermName, Dictionary, Language, Audience, ApiVers, object
				from dbo.Dictionary
				where TermID = @TermID
				  and Language = @Language
				   and ApiVers = @ApiVers


END
GO

USE pdq_dictionaries
GO

/****** Object:  StoredProcedure [dbo].[usp_SearchDictionary]    Script Date: 6/23/2020 2:35:06 PM ******/
SET ANSI_NULLS ON
GO

SET QUOTED_IDENTIFIER ON
GO


-- Returns a set of table containing up to @maxResults rows of
-- data mathing the specified search criteria

CREATE PROCEDURE [dbo].[usp_SearchDictionary]
	@searchText nvarchar(2000), -- The text to search for
	@Dictionary nvarchar(10), -- The dictionary to search (term/drug/genetic)
	@Language nvarchar(20), -- The language to use for search and results
	@Audience nvarchar(25), -- Target audience.
	@ApiVers nvarchar(10), -- What version of the API? (There may be multiple.)
	@offset int = 0, -- Zero-based offset into the list of results for the first record to return
	@maxResults int = 200, -- The maximum number of results to return

	@matchCount int out -- Returns the total number of rows matching the criteria.

AS
BEGIN

-- spanish search uses spanish accent insensive locale.

IF @Language = 'English'
	BEGIN
		-- Query for the search results, keeping at most @maxResults records.
		select top(@maxResults) row, TermID, TermName, Dictionary, Language, Audience, ApiVers, object
		from
		(
			select ROW_NUMBER() over (order by TermName ) as row, *
			from Dictionary with (nolock)
			where Dictionary = @dictionary
			  and Language = @language
			  and Audience = @Audience
			  and ApiVers = @ApiVers
			  and (TermName like @searchText
				   or TermID in
					(select termid from DictionaryTermAlias where Othername like @searchText)
				)
		) filtered
		where row > @offset -- Use > because offset is zero-based but row is one-based
		order by row

		-- To get the total match count, we re-run the query without a row limit.
		select @matchCount = COUNT(*)
		from Dictionary with (nolock)
		where Dictionary = @dictionary
		  and Language = @language
		  and Audience = @Audience
		  and ApiVers = @ApiVers
		  and (TermName like @searchText
			   or TermID in
				(select termid from DictionaryTermAlias where Othername like @searchText)
			)
	END
ELSE
	BEGIN
		-- Query for the search results, keeping at most @maxResults records.
		select top(@maxResults) row, TermID, TermName, Dictionary, Language, Audience, ApiVers, object
		from
		(
			select ROW_NUMBER() over (order by TermName  collate modern_spanish_CI_AI) as row, *
			from Dictionary with (nolock)
			where Dictionary = @dictionary
			  and Language = @language
			  and Audience = @Audience
			  and ApiVers = @ApiVers
			  and (TermName collate modern_spanish_CI_AI  like @searchText
				   or TermID in
					(select termid from DictionaryTermAlias where Othername  collate modern_spanish_CI_AI  like @searchText)
				)
		) filtered
		where row > @offset -- Use > because offset is zero-based but row is one-based
		order by row

		-- To get the total match count, we re-run the query without a row limit.
		select @matchCount = COUNT(*)
		from Dictionary with (nolock)
		where Dictionary = @dictionary
		  and Language = @language
		  and Audience = @Audience
		  and ApiVers = @ApiVers
		  and (TermName collate modern_spanish_CI_AI  like @searchText
			   or TermID in
				(select termid from DictionaryTermAlias where Othername   collate modern_spanish_CI_AI  like @searchText)
			)
	END


END
GO

USE pdq_dictionaries
GO

/****** Object:  StoredProcedure [dbo].[usp_SearchSuggestDictionary]    Script Date: 6/23/2020 2:35:50 PM ******/
SET ANSI_NULLS ON
GO

SET QUOTED_IDENTIFIER ON
GO


-- Retursn a table containing term names or aliases which match the
-- specified search criteria

CREATE PROCEDURE [dbo].[usp_SearchSuggestDictionary]
	@searchText nvarchar(1000), -- text to match
	@searchType nvarchar(11), -- 'begins', 'contains' or 'magic' (begins followed by contains)
	@Dictionary nvarchar(10), -- The dictionary to search (term/drug/genetic)
	@Language nvarchar(20), -- The language to use for search and results
	@Audience nvarchar(25), -- Target audience.
	@ApiVers nvarchar(10), -- What version of the API? (There may be multiple.)
	@maxResults int = 10, -- The maximum number of results to return

	@matchCount int out -- Returns the total number of rows matching the criteria.

AS
BEGIN

-- Temporary table to hold the search results.
create table #resultSet(
	searchType nvarchar(10),
	TermID int,
	TermName nvarchar(1000)
);

-- Separate comparison values for begins vs contains. Doing this here vs. the
-- middle tier allows us to do the "magic" search with the same set of queries..
declare @beginsTerm nvarchar(1000),
		@containsTerm nvarchar(1000)

select @beginsTerm = @searchText + '%';
select @containsTerm = '%[- ]' + @searchText + '%';

-- Gather up the full set of results.  Don't worry about
-- sorting or limiting to @maxResults yet because we need to
-- find out the total number of possible matches too.
-- Prepending begins vs contains provides a means of ordering them
-- for the "magic" search type.

-- spanish search uses spanish accent insensive locale.

IF @Language = 'english'
		BEGIN
			insert #resultSet (searchType, TermID, TermName)
		(
			-- Match term name beginning with @beginsTerm
			select 'begins' as searchType, TermID, TermName
			from Dictionary with (nolock)
			where TermName like @beginsTerm
			  and Language = @language
			  and Dictionary = @dictionary
			  and (@searchType = 'magic' or @searchType = 'begins')

		  union

			-- Match alternate name beginning with @beginsTerm
			select 'begins' as searchType, d.TermID, dta.OtherName
			from Dictionary d with (nolock) join DictionaryTermAlias dta with (nolock) on d.TermID = dta.TermID
			where dta.OtherName like @beginsTerm
			  and d.Language = @language
			  and d.Dictionary = @dictionary
			  and (@searchType = 'magic' or @searchType = 'begins')

		  union

			-- Match term name containing @containsTerm
			select 'contains' as searchType, TermID, TermName
			from Dictionary with (nolock)
			where TermName like @containsTerm
			  and Language = @language
			  and Dictionary = @dictionary
			  and (@searchType = 'magic' or @searchType = 'contains')
			  and TermID not in (select TermID from #resultSet)

		union

			-- Match alternate name containing @containsTerm
			select 'contains' as searchType, d.TermID, dta.OtherName
			from Dictionary d with (nolock) join DictionaryTermAlias dta with (nolock) on d.TermID = dta.TermID
				and dta.Othername not in(select TermName from #resultSet)
			where dta.OtherName like @containsTerm
			  and d.Language = @language
			  and d.Dictionary = @dictionary
			  and (@searchType = 'magic' or @searchType = 'begins')
		)
	END
	ELSE
		BEGIN
			insert #resultSet (searchType, TermID, TermName)
			(
				-- Match term name beginning with @beginsTerm
				select 'begins' as searchType, TermID, TermName
				from Dictionary with (nolock)
				where TermName  collate modern_spanish_CI_AI  like @beginsTerm
				  and Language = @language
				  and Dictionary = @dictionary
				  and (@searchType = 'magic' or @searchType = 'begins')

			  union

				-- Match alternate name beginning with @beginsTerm
				select 'begins' as searchType, d.TermID, dta.OtherName
				from Dictionary d with (nolock) join DictionaryTermAlias dta with (nolock) on d.TermID = dta.TermID
				where dta.OtherName  collate modern_spanish_CI_AI  like @beginsTerm
				  and d.Language = @language
				  and d.Dictionary = @dictionary
				  and (@searchType = 'magic' or @searchType = 'begins')

			  union

				-- Match term name containing @containsTerm
				select 'contains' as searchType, TermID, TermName
				from Dictionary with (nolock)
				where TermName  collate modern_spanish_CI_AI  like @containsTerm
				  and Language = @language
				  and Dictionary = @dictionary
				  and (@searchType = 'magic' or @searchType = 'contains')
				  and TermID not in (select TermID from #resultSet)

			union

				-- Match alternate name containing @containsTerm
				select 'contains' as searchType, d.TermID, dta.OtherName
				from Dictionary d with (nolock) join DictionaryTermAlias dta with (nolock) on d.TermID = dta.TermID
					and dta.Othername not in(select TermName from #resultSet)
				where dta.OtherName  collate modern_spanish_CI_AI  like @containsTerm
				  and d.Language = @language
				  and d.Dictionary = @dictionary
				  and (@searchType = 'magic' or @searchType = 'begins')
			)
	END

-- Get the number of results matching the search criteria.
select @matchCount = @@RowCount


-- Return the requested search results.
select top (@maxResults) row, TermID, TermName --, Dictionary, Language, Audience, ApiVers, object
from
(
	select ROW_NUMBER() over (order by searchType, termName) as row, *
	from #resultSet
) oresult







END
GO

USE pdq_dictionaries
GO

/****** Object:  StoredProcedure [dbo].[usp_SearchExpandDictionary]    Script Date: 6/23/2020 2:36:43 PM ******/
SET ANSI_NULLS ON
GO

SET QUOTED_IDENTIFIER ON
GO


-- Retursn a table containing term names or aliases which match the
-- specified search criteria

CREATE PROCEDURE [dbo].[usp_SearchExpandDictionary]
	@searchText nvarchar(1000), -- text to match
	@IncludeTypes udt_DictionaryAliasTypeFilter READONLY, -- List of otherNameType values to include
	@Dictionary nvarchar(10), -- The dictionary to search (term/drug/genetic)
	@Language nvarchar(20), -- The language to use for search and results
	@Audience nvarchar(25), -- Target audience.
	@ApiVers nvarchar(10), -- What version of the API? (There may be multiple.)
	@offset int = 0, -- Zero-based offset into the list of results for the first record to return
	@maxResults int = 10, -- The maximum number of results to return

	@matchCount int out -- Returns the total number of rows matching the criteria.

AS
BEGIN

-- Temporary table to hold the search results.  Object will be added in
-- later via a JOIN.
create table #resultSet(
	TermID int,
	MatchedTermName nvarchar(1000)
);


-- If no includes were specified, then include everything.
declare @filter_count int;
declare @filter udt_DictionaryAliasTypeFilter;

select @filter_count = count(*) from @IncludeTypes
if(@filter_count = 0)
begin
	-- No JOIN, so this will get everything. Aliases only exist for drugs,
	-- so this won't matter much.
	insert into @filter(NameType)
	select distinct otherNameType
	from dictionaryTermAlias
end
else
begin
	insert into @filter(NameType)
	select nameType
	from @IncludeTypes
end


--# for name starting with number and symbol
if @searchText = '[0-9]'
	BEGIN
		insert into #resultSet(Termid, matchedTErmName)
		select termid , termname
		from dbo.dictionary with (nolock)
		where termname not like '[a-zA-Z]%'
		and language = @Language
		and Dictionary = @Dictionary
		union
		select d.TermID, dta.OtherName
		from Dictionary d with (nolock) join DictionaryTermAlias dta with (nolock) on d.TermID = dta.TermID
			and dta.OtherNameType in (select NameType from @filter)
		where dta.OtherName not like '[a-zA-Z]%'
			and d.Language = @language
			and d.Dictionary = @dictionary

	END

ELSE

	BEGIN
		-- Expand is always based on a begins with match the first letter.
		select @searchText = @searchText + '%';


		-- Gather up the full set of results.  Don't worry about
		-- sorting or limiting to @maxResults yet because we need to
		-- find out the total number of possible matches too.
		-- Prepending begins vs contains provides a means of ordering them
		-- for the "magic" search type.

		-- spanish accent insensitive

		IF @language = 'english'
				BEGIN
					insert #resultSet (TermID, MatchedTermName)
					(
						-- Match term name beginning with @searchText
						select TermID, TermName
						from Dictionary with (nolock)
						where TermName like @searchText
						  and Language = @language
						  and Dictionary = @dictionary

					  union

						-- Match alternate name beginning with @searchText
						select d.TermID, dta.OtherName
						from Dictionary d with (nolock) join DictionaryTermAlias dta with (nolock) on d.TermID = dta.TermID
							and dta.OtherNameType in (select NameType from @filter)
						where dta.OtherName like @searchText
						  and d.Language = @language
						  and d.Dictionary = @dictionary
					)
				END
			ELSE
				BEGIN
					insert #resultSet (TermID, MatchedTermName)
					(
						-- Match term name beginning with @searchText
						select TermID, TermName
						from Dictionary with (nolock)
						where TermName  collate modern_spanish_CI_AI like @searchText
						  and Language = @language
						  and Dictionary = @dictionary

					  union

						-- Match alternate name beginning with @searchText
						select d.TermID, dta.OtherName
						from Dictionary d with (nolock) join DictionaryTermAlias dta with (nolock) on d.TermID = dta.TermID
							and dta.OtherNameType in (select NameType from @filter)
						where dta.OtherName  collate modern_spanish_CI_AI  like @searchText
						  and d.Language = @language
						  and d.Dictionary = @dictionary
					)
				END
	 END
-- Get the number of results matching the search criteria.
select @matchCount = @@RowCount


-- Return the requested search results.
select top (@maxResults) row, TermID, TermName, object
from
(
	select ROW_NUMBER() over (order by rs.MatchedTermName) as row, rs.TermId, rs.MatchedTermName as TermName, d.Object
	from #resultSet rs join dictionary d on rs.termid = d.termid
) oresult
where row > @offset -- Use > because offset is zero-based but row is one-based

END
GO

USE pdq_dictionaries
GO

/****** Object:  StoredProcedure [dbo].[dictionaryTerm_exist]    Script Date: 6/23/2020 2:37:33 PM ******/
SET ANSI_NULLS ON
GO

SET QUOTED_IDENTIFIER ON
GO

create procedure [dbo].[dictionaryTerm_exist](@dictionaryTerms udt_dictionaryTerm readonly)
As
BEGIN
	set nocount on
	select t.cdrid, t.dictionarytype, t.language, t.audience, case when d.termid is null then 0 else 1 end
	from @dictionaryterms t left outer join dbo.dictionary d on d.termid = t.cdrid and d.Dictionary = t.dictionarytype and d.Language = t.language and d.audience = t.audience


END

GO

/* Allow the website user to invoke the stored procs. */
GRANT EXECUTE ON usp_GetDictionaryTerm TO websiteuser
GRANT EXECUTE ON usp_GetDictionaryTermForAudience TO websiteuser
GRANT EXECUTE ON usp_SearchDictionary TO websiteuser
GRANT EXECUTE ON usp_SearchSuggestDictionary TO websiteuser
GRANT EXECUTE ON usp_SearchExpandDictionary TO websiteuser
GRANT EXECUTE ON dictionaryTerm_exist TO websiteuser
GRANT EXECUTE ON TYPE::udt_DictionaryAliasTypeFilter TO websiteuser
GRANT EXECUTE ON TYPE::udt_DictionaryTerm TO websiteuser
GO
