#----------------------------------------------------------------------
# Mark document types we no longer need as inactive.
#----------------------------------------------------------------------
import cdrdb

conn = cdrdb.connect()
cursor = conn.cursor()
cursor.execute("""\
UPDATE doc_type
   SET active = 'N'
 WHERE name IN ('css',
                'CTRPProtocol',
                'GENETICSPROFESSIONAL',
                'GlossaryTerm',
                'InScopeProtocol',
                'OutOfScopeProtocol',
                'ScientificProtocolInfo')""")
conn.commit()
print "obsolete document types inactivated ..."
