/*
 * $Id: cdrdata.sql,v 1.5 2000-10-10 20:04:08 mruben Exp $
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.4  2000/04/26 01:44:17  bkline
 * Fixed schema columns.
 *
 * Revision 1.3  2000/04/21 19:49:25  bkline
 * Added data for testing group actions.
 *
 * Revision 1.2  2000/04/17 21:34:02  bkline
 * Data mods to test CdrCheckAuth command.
 *
 * Revision 1.1  2000/04/14 14:04:58  bkline
 * Initial revision
 *
 */

INSERT INTO usr(name, password, created) VALUES('mmr', '***REDACTED***', GETDATE())
INSERT INTO usr(name, password, created) VALUES('rmk', '***REDACTED***', GETDATE())
INSERT INTO usr(name, password, created) VALUES('ahm', '***REDACTED***', GETDATE())
go

INSERT INTO format(name) VALUES('none')
INSERT INTO format(name) VALUES('xml')
INSERT INTO format(name) VALUES('html')
INSERT INTO format(name) VALUES('worddoc')
INSERT INTO format(name) VALUES('jpg')
INSERT INTO format(name) VALUES('pgn')
INSERT INTO format(name) VALUES('smtp')
go

INSERT INTO doc_type(name, format, created, versioning, dtd, xml_schema)
VALUES('', 2, GETDATE(), 'N', '', '')
INSERT INTO doc_type(name, format, created, versioning, dtd, xml_schema)
VALUES('Person', 1, GETDATE(), 'Y',
       '<xsd:schema xmlns:xsd="http://www.w3.org/1999/XMLSchema">\n' +
       '<!ELEMENT Person     (FirstName, LastName)>\n' +
       '<!ELEMENT FirstName  (#PCDATA)>\n' +
       '<!ELEMENT LastName   (#PCDATA)>\n',
       '<xsd:element     name="Person" type="Person"/>\n' +
       '<xsd:complexType name="Person">\n' +
       '    <xsd:element name="FirstName" type="xsd:string"/>\n' +
       '    <xsd:element name="LastName"  type="xsd:string"/>\n' +
       '</xsd:complexType>\n' +
       '</xsd:schema>')
INSERT INTO doc_type(name, format, created, versioning, dtd, xml_schema)
VALUES('Org', 2, GETDATE(), 'Y',
       '<!ELEMENT Org        (Name, Address)>\n' +
       '<!ELEMENT Name       (#PCDATA)>\n' +
       '<!ELEMENT Address    (#PCDATA)>\n',
       '<xsd:schema xmlns:xsd="http://www.w3.org/1999/XMLSchema">\n' +
       '<xsd:element     name="Org"       type="Org"/>\n' +
       '<xsd:complexType name="Org">\n' +
       '    <xsd:element name="Name"      type="xsd:string"/>\n' +
       '    <xsd:element name="Address"   type="xsd:string"/>\n' +
       '</xsd:complexType>\n' +
       '</xsd:schema>')
go

INSERT INTO doc_status(id, name) VALUES('U', 'UNVALIDATED')
INSERT INTO doc_status(id, name) VALUES('I', 'INVALID')
INSERT INTO doc_status(id, name) VALUES('V', 'VALID')
INSERT INTO doc_status(id, name) VALUES('P', 'PUBLISHED')
go

INSERT INTO document(created, creator, doc_type, title, xml)
VALUES(GETDATE(), 1, 2, 'James Bond', 
  '<Person><FirstName>James</FirstName><LastName>Bond</LastName></Person>')
INSERT INTO document(created, creator, doc_type, title, xml)
VALUES(GETDATE(), 2, 2, 'Sam Spade', 
  '<Person><FirstName>Sam</FirstName><LastName>Spade</LastName></Person>')
INSERT INTO document(created, creator, doc_type, title, xml)
VALUES(GETDATE(), 3, 2, 'George Smiley', 
  '<Person><FirstName>George</FirstName><LastName>Smiley</LastName></Person>')
INSERT INTO document(created, creator, doc_type, title, xml)
VALUES(GETDATE(), 1, 2, 'Sherlock Holmes', 
  '<Person><FirstName>Sherlock</FirstName><LastName>Holmes</LastName></Person>')
go

INSERT INTO attr(id, name, num, val) VALUES(1, 'OCCUPATION', 1, 'AGENT')
INSERT INTO attr(id, name, num, val) VALUES(2, 'OCCUPATION', 1, 'DETECTIVE')
INSERT INTO attr(id, name, num, val) VALUES(3, 'OCCUPATION', 1, 'SPY')
INSERT INTO attr(id, name, num, val) VALUES(4, 'OCCUPATION', 1, 'DETECTIVE')
go

INSERT INTO action(name) VALUES('ADD DOCUMENT')
INSERT INTO action(name) VALUES('MODIFY DOCUMENT')
INSERT INTO action(name) VALUES('DELETE DOCUMENT')
INSERT INTO action(name) VALUES('APPROVE DOCUMENT')
INSERT INTO action(name) VALUES('PUBLISH DOCUMENT')
INSERT INTO action(name) VALUES('CREATE USER')
INSERT INTO action(name) VALUES('DELETE USER')
INSERT INTO action(name) VALUES('MODIFY USER')
INSERT INTO action(name) VALUES('ADD GROUP')
INSERT INTO action(name) VALUES('MODIFY GROUP')
INSERT INTO action(name) VALUES('DELETE GROUP')
INSERT INTO action(name) VALUES('GET GROUP')
INSERT INTO action(name) VALUES('LIST GROUPS')
INSERT INTO action(name) VALUES('GET USER')
INSERT INTO action(name) VALUES('VALIDATE DOCUMENT')
INSERT INTO action(name) VALUES('FORCE CHECKIN')
INSERT INTO action(name) VALUES('FORCE CHECKOUT')
go

INSERT INTO grp(name) VALUES('TRAINEES')
INSERT INTO grp(name) VALUES('EDITORS')
INSERT INTO grp(name) VALUES('MANAGERS')
INSERT INTO grp(name) VALUES('ORG EDITORS')
go

INSERT INTO grp_action(grp, action, doc_type) VALUES(1, 1, 2)
INSERT INTO grp_action(grp, action, doc_type) VALUES(1, 2, 2)
INSERT INTO grp_action(grp, action, doc_type) VALUES(2, 1, 2)
INSERT INTO grp_action(grp, action, doc_type) VALUES(2, 2, 2)
INSERT INTO grp_action(grp, action, doc_type) VALUES(2, 3, 2)
INSERT INTO grp_action(grp, action, doc_type) VALUES(3, 1, 2)
INSERT INTO grp_action(grp, action, doc_type) VALUES(3, 2, 2)
INSERT INTO grp_action(grp, action, doc_type) VALUES(3, 3, 2)
INSERT INTO grp_action(grp, action, doc_type) VALUES(3, 4, 2)
INSERT INTO grp_action(grp, action, doc_type) VALUES(3, 5, 2)
INSERT INTO grp_action(grp, action, doc_type) VALUES(3, 6, 1)
INSERT INTO grp_action(grp, action, doc_type) VALUES(3, 7, 1)
INSERT INTO grp_action(grp, action, doc_type) VALUES(3, 8, 1)
INSERT INTO grp_action(grp, action, doc_type) VALUES(3, 9, 1)
INSERT INTO grp_action(grp, action, doc_type) VALUES(3, 10, 1)
INSERT INTO grp_action(grp, action, doc_type) VALUES(3, 11, 1)
INSERT INTO grp_action(grp, action, doc_type) VALUES(3, 12, 1)
INSERT INTO grp_action(grp, action, doc_type) VALUES(3, 13, 1)
INSERT INTO grp_action(grp, action, doc_type) VALUES(3, 14, 1)
INSERT INTO grp_action(grp, action, doc_type) VALUES(3, 15, 2)
INSERT INTO grp_action(grp, action, doc_type) VALUES(4, 1, 3)
INSERT INTO grp_action(grp, action, doc_type) VALUES(4, 2, 3)
INSERT INTO grp_action(grp, action, doc_type) VALUES(4, 3, 3)
go

INSERT INTO grp_usr(grp, usr) VALUES(1, 2)
INSERT INTO grp_usr(grp, usr) VALUES(2, 1)
INSERT INTO grp_usr(grp, usr) VALUES(1, 3)
INSERT INTO grp_usr(grp, usr) VALUES(4, 3)
INSERT INTO grp_usr(grp, usr) VALUES(3, 1)
go
