/*
 * $Id: cdrdata.sql,v 1.1 2000-04-14 14:04:58 bkline Exp $
 *
 * $Log: not supported by cvs2svn $
 */

INSERT INTO usr(name, password, created) VALUES('mmr', '***REDACTED***', GETDATE())
INSERT INTO usr(name, password, created) VALUES('rmk', '***REDACTED***', GETDATE())
INSERT INTO usr(name, password, created) VALUES('ahm', '***REDACTED***', GETDATE())
go

INSERT INTO format(name) VALUES('xml')
INSERT INTO format(name) VALUES('html')
INSERT INTO format(name) VALUES('worddoc')
INSERT INTO format(name) VALUES('jpg')
INSERT INTO format(name) VALUES('pgn')
INSERT INTO format(name) VALUES('smtp')
go

INSERT INTO doc_type(name, format, created, versioning, dtd, xml_schema)
VALUES('Person', 1, GETDATE(), 'Y',
       '<!ELEMENT Person     (FirstName, LastName)>\n' +
       '<!ELEMENT FirstName  (#PCDATA)>\n' +
       '<!ELEMENT LastName   (#PCDATA)>\n',
       '<xsd:element     name="Person" type="Person"/>\n' +
       '<xsd:complexType name="Person">\n' +
       '    <xsd:element name="FirstName" type="xsd:string"/>\n' +
       '    <xsd:element name="LastName"  type="xsd:string"/>\n' +
       '</xsd:complexType>')
go

INSERT INTO doc_status(id, name) VALUES('U', 'UNVALIDATED')
INSERT INTO doc_status(id, name) VALUES('I', 'INVALID')
INSERT INTO doc_status(id, name) VALUES('V', 'VALID')
INSERT INTO doc_status(id, name) VALUES('P', 'PUBLISHED')
go

INSERT INTO document(created, creator, doc_type, title, xml)
VALUES(GETDATE(), 1, 1, 'James Bond', 
  '<Person><FirstName>James</FirstName><LastName>Bond</LastName></Person>')
INSERT INTO document(created, creator, doc_type, title, xml)
VALUES(GETDATE(), 2, 1, 'Sam Spade', 
  '<Person><FirstName>Sam</FirstName><LastName>Spade</LastName></Person>')
INSERT INTO document(created, creator, doc_type, title, xml)
VALUES(GETDATE(), 3, 1, 'George Smiley', 
  '<Person><FirstName>George</FirstName><LastName>Smiley</LastName></Person>')
INSERT INTO document(created, creator, doc_type, title, xml)
VALUES(GETDATE(), 1, 1, 'Sherlock Holmes', 
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
go

INSERT INTO grp(name) VALUES('TRAINEES')
INSERT INTO grp(name) VALUES('EDITORS')
INSERT INTO grp(name) VALUES('MANAGERS')
go

INSERT INTO grp_action(grp, action, doc_type) VALUES(1, 1, 1)
INSERT INTO grp_action(grp, action, doc_type) VALUES(1, 2, 1)
INSERT INTO grp_action(grp, action, doc_type) VALUES(2, 1, 1)
INSERT INTO grp_action(grp, action, doc_type) VALUES(2, 2, 1)
INSERT INTO grp_action(grp, action, doc_type) VALUES(2, 3, 1)
INSERT INTO grp_action(grp, action, doc_type) VALUES(3, 1, 1)
INSERT INTO grp_action(grp, action, doc_type) VALUES(3, 2, 1)
INSERT INTO grp_action(grp, action, doc_type) VALUES(3, 3, 1)
INSERT INTO grp_action(grp, action, doc_type) VALUES(3, 4, 1)
INSERT INTO grp_action(grp, action, doc_type) VALUES(3, 5, 1)
go

INSERT INTO grp_usr(grp, usr) VALUES(1, 2)
INSERT INTO grp_usr(grp, usr) VALUES(2, 1)
INSERT INTO grp_usr(grp, usr) VALUES(1, 3)
go
