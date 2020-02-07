#!/usr/bin/env python3

"""
Run these tests whenever changes are made to the CDR filters

The numbers in the names guarantee the order in which the tests will be run.
The extra underscores in the names make the verbose output much easier to read.
"""

import unittest
from lxml import etree
from cdrapi.settings import Tier
from cdrapi.docs import Doc, Filter, FilterSet
from cdrapi.searches import Search
from cdrapi.users import Session
from cdrapi import db

try:
    str
except:
    str = (str, bytes)
    str = str


class Tests(unittest.TestCase):
    USERNAME = "tester"
    COVERED = set()
    CURSOR = db.connect().cursor()

    def setUp(self):
        password = Tier().password(self.USERNAME)
        opts = dict(comment="filter testing", password=password)
        Tests.session = Session.create_session(self.USERNAME, **opts)

    def tearDown(self):
        self.session.logout()

    def filter(self, doc, filter_spec, **opts):
        prefix, name = filter_spec.split(":", 1)
        if prefix == "name":
            doc_id = Doc.id_from_title(name)
            self.register_filter(Doc(self.session, id=doc_id))
        else:
            self.register_filter_set(name)
        return doc.filter(filter_spec, **opts)

    def register_filter(self, filter_doc):
        if not filter_doc.title in Tests.COVERED:
            Tests.COVERED.add(filter_doc.title)
            xml = filter_doc.get_filter(filter_doc.id).xml
            root = etree.fromstring(xml)
            for name in ("import", "include"):
                qname = Doc.qname(name, Filter.NS)
                for node in root.iter(qname):
                    href = node.get("href").replace("%20", " ")
                    if href.startswith("cdr:name:"):
                        title = href.split(":name:", 1)[1]
                        doc_id = Doc.id_from_title(title)
                        self.register_filter(Doc(self.session, id=doc_id))

    def register_filter_set(self, name):
        filter_set = FilterSet(self.session, name=name)
        for member in filter_set.members:
            if isinstance(member, Doc):
                self.register_filter(member)
            else:
                self.register_filter_set(member.name)

    def get_doc(self, doctype):
        query = db.Query("document d", "d.id").limit(1).order("d.id")
        query.join("doc_type t", "t.id = d.doc_type")
        query.where(query.Condition("t.name", doctype))
        row = query.execute(self.CURSOR).fetchone()
        return Doc(self.session, id=row.id)

    def get_published_doc(self, doctype):
        fields = "d.doc_id", "d.doc_version"
        query = db.Query("pub_proc_doc d", *fields).limit(1).order("d.doc_id")
        query.join("pub_proc p", "p.id = d.pub_proc")
        query.join("doc_version v", "v.id = d.doc_id", "v.num = d.doc_version")
        query.join("doc_type t", "t.id = v.doc_type")
        query.where(query.Condition("t.name", doctype))
        row = query.execute(self.CURSOR).fetchone()
        return Doc(self.session, id=row.doc_id, version=row.doc_version)

    def get_published_xml(self, doc_id):
        query = db.Query("pub_proc_cg", "xml")
        query.where(query.Condition("id", doc_id))
        return query.execute(self.CURSOR).fetchone().xml

    def normalize(xml):
        return re.sub("<?xml[^>]?>", "", xml).strip()

# Set FULL to False temporarily when adding new tests so you can get
# the new ones working without having to grind through the entire set.

FULL = True
#FULL = False
if FULL:

    class _01TitleGeneration(Tests):

        """
        """
        def test_001_citation_tit(self):
            root = etree.Element("Citation")
            wrapper = etree.SubElement(root, "PubmedArticle")
            wrapper = etree.SubElement(wrapper, "MedlineCitation")
            wrapper = etree.SubElement(wrapper, "Article")
            etree.SubElement(wrapper, "ArticleTitle").text = "x" * 512
            doc = Doc(self.session, xml=etree.tostring(root))
            result = self.filter(doc, "name:DocTitle for Citation")
            self.assertFalse(result.error_log)
            self.assertFalse(result.messages)
            # The author of the filter thought she was getting 255
            # characters (according to her comment) but the expression
            # 'substring(string(.), 0, 255)' yields at most 254 characters.
            self.assertEqual(str(result.result_tree), "x" * 254)
        def test_002_clinical_tri(self):
            root = etree.Element("ClinicalTrialSearchString")
            etree.SubElement(root, "SearchString").text = "dada"
            doc = Doc(self.session, xml=etree.tostring(root))
            filter = "name:DocTitle for ClinicalTrialSearchString"
            result = self.filter(doc, filter)
            self.assertFalse(result.error_log)
            self.assertFalse(result.messages)
            self.assertEqual(str(result.result_tree), "dada")
        def test_003_country_titl(self):
            root = etree.Element("Country")
            etree.SubElement(root, "CountryFullName").text = "gimte"
            doc = Doc(self.session, xml=etree.tostring(root))
            result = self.filter(doc, "name:DocTitle for Country")
            self.assertFalse(result.error_log)
            self.assertFalse(result.messages)
            self.assertEqual(str(result.result_tree), "gimte")
        def test_004_ctgovprotoco(self):
            doc = self.get_doc("CTGovProtocol")
            result = self.filter(doc, "name:DocTitle for CTGovProtocol")
            self.assertFalse(result.error_log)
            self.assertFalse(result.messages)
            self.assertTrue(len(str(result.result_tree)) < 256)
        def test_005_documentatio(self):
            root = etree.Element("Documentation")
            wrapper = etree.SubElement(root, "Body")
            etree.SubElement(wrapper, "DocumentationTitle").text = "part 1"
            wrapper = etree.SubElement(root, "Metadata")
            etree.SubElement(wrapper, "Function").text = "part 2"
            etree.SubElement(wrapper, "DocType").text = "part 3"
            root.set("InfoType", "part 4")
            expected = ";".join(["part {}".format(i+1) for i in range(4)])
            doc = Doc(self.session, xml=etree.tostring(root))
            result = self.filter(doc, "name:DocTitle for Documentation")
            self.assertFalse(result.error_log)
            self.assertFalse(result.messages)
            self.assertEqual(str(result.result_tree), expected)
        def test_006_doctoc_title(self):
            root = etree.Element("DocumentationToC")
            etree.SubElement(root, "ToCTitle").text = "test title"
            root.set("Use", "test use")
            doc = Doc(self.session, xml=etree.tostring(root))
            result = self.filter(doc, "name:DocTitle for DocumentationToC")
            self.assertFalse(result.error_log)
            self.assertFalse(result.messages)
            self.assertEqual(str(result.result_tree), "test title;test use")
        def test_007_druginformat(self):
            root = etree.Element("DrugInformationSummary")
            etree.SubElement(root, "Title").text = "A"
            wrapper = etree.SubElement(root, "DrugInfoMetaData")
            etree.SubElement(wrapper, "DrugInfoType").text = "B"
            etree.SubElement(wrapper, "Audience").text = "C"
            doc = Doc(self.session, xml=etree.tostring(root))
            filter = "name:DocTitle for DrugInformationSummary"
            result = self.filter(doc, filter)
            self.assertFalse(result.error_log)
            self.assertFalse(result.messages)
            self.assertEqual(str(result.result_tree), ";".join("ABC"))
        def test_008_geneticsprof(self):
            root = etree.Element("GENETICSPROFESSIONAL")
            wrapper = etree.SubElement(root, "NAME")
            etree.SubElement(wrapper, "LASTNAME").text = "Kadiddlehopper"
            etree.SubElement(wrapper, "FIRSTNAME").text = "Klem"
            doc = Doc(self.session, xml=etree.tostring(root))
            result = self.filter(doc, "name:DocTitle for GENETICSPROFESSIONAL")
            self.assertFalse(result.error_log)
            self.assertFalse(result.messages)
            self.assertEqual(str(result.result_tree), "Kadiddlehopper, Klem")
        def test_009_glossaryterm(self):
            root = etree.Element("GlossaryTerm")
            etree.SubElement(root, "TermName").text = "tEsT NaMe"
            doc = Doc(self.session, xml=etree.tostring(root))
            result = self.filter(doc, "name:DocTitle for GlossaryTerm")
            self.assertFalse(result.error_log)
            self.assertFalse(result.messages)
            self.assertEqual(str(result.result_tree), "tEsT NaMe")
        def test_010_gtconcept_ti(self):
            doc = self.get_doc("GlossaryTermConcept")
            result = self.filter(doc, "name:DocTitle for GlossaryTermConcept")
            expected = "{}: ".format(doc.id)
            self.assertFalse(result.error_log)
            self.assertFalse(result.messages)
            self.assertTrue(str(result.result_tree).startswith(expected))
        def test_011_gtname_title(self):
            root = etree.Element("GlossaryTermName")
            wrapper = etree.SubElement(root, "TermName")
            etree.SubElement(wrapper, "TermNameString").text = "Yo!"
            wrapper = etree.SubElement(root, "TranslatedName")
            etree.SubElement(wrapper, "TermNameString").text = "\xa1Hola!"
            doc = Doc(self.session, xml=etree.tostring(root))
            expected = "Yo!;\xa1Hola! [es]"
            result = self.filter(doc, "name:DocTitle for GlossaryTermName")
            self.assertFalse(result.error_log)
            self.assertFalse(result.messages)
            self.assertEqual(str(result.result_tree), expected)
        def test_012_inscopeproto(self):
            root = etree.Element("InScopeProtocol")
            protocol_ids = etree.SubElement(root, "ProtocolIDs")
            wrapper = etree.SubElement(protocol_ids, "PrimaryID")
            etree.SubElement(wrapper, "IDString").text = "A"
            for id in "BCDE":
                wrapper = etree.SubElement(protocol_ids, "OtherID")
                etree.SubElement(wrapper, "IDString").text = id
            type_names = "Original", "Patient", "Professional"
            for i, type_name in enumerate(type_names):
                title = etree.SubElement(root, "ProtocolTitle")
                title.set("Type", type_name)
                title.text = "xyz"[i] * 1024
            doc = Doc(self.session, xml=etree.tostring(root))
            result = self.filter(doc, "name:DocTitle for InScopeProtocol")
            expected = "{};{}".format(";".join("ABCDE"), "z" * 1024)[:255]
            self.assertFalse(result.error_log)
            self.assertFalse(result.messages)
            self.assertEqual(str(result.result_tree), expected)
        def test_013_licensee_tit(self):
            root = etree.Element("Licensee")
            info = etree.SubElement(root, "LicenseeInformation")
            name_info = etree.SubElement(info, "LicenseeNameInformation")
            wrapper = etree.SubElement(name_info, "OfficialName")
            etree.SubElement(wrapper, "Name").text = "Dada, G.m.b.H."
            etree.SubElement(info, "LicenseeStatus").text = "Test"
            doc = Doc(self.session, xml=etree.tostring(root))
            result = self.filter(doc, "name:DocTitle for Licensee")
            self.assertFalse(result.error_log)
            self.assertFalse(result.messages)
            self.assertEqual(str(result.result_tree), "Dada, G.m.b.H.;Test")
        def test_014_mailer_title(self):
            doc = self.get_doc("Mailer")
            result = self.filter(doc, "name:DocTitle for Mailer")
            self.assertFalse(result.error_log)
            self.assertFalse(result.messages)
            self.assertTrue("[" in str(result.result_tree))
            self.assertTrue("]" in str(result.result_tree))
        def test_015_media_title_(self):
            root = etree.Element("Media")
            etree.SubElement(root, "MediaTitle").text = "gimte"
            wrapper = etree.SubElement(root, "MediaContent")
            wrapper = etree.SubElement(wrapper, "Categories")
            etree.SubElement(wrapper, "Category").text = "dada"
            wrapper = etree.SubElement(root, "PhysicalMedia")
            wrapper = etree.SubElement(wrapper, "SoundData")
            etree.SubElement(wrapper, "SoundEncoding").text = "MP3"
            doc = Doc(self.session, xml=etree.tostring(root))
            result = self.filter(doc, "name:DocTitle for Media")
            self.assertFalse(result.error_log)
            self.assertFalse(result.messages)
            self.assertEqual(str(result.result_tree), "gimte;dada;MP3")
        def test_016_miscdoc_titl(self):
            root = etree.Element("MiscellaneousDocument")
            etree.SubElement(root, "MiscellaneousDocumentTitle").text = "dada"
            wrapper = etree.SubElement(root, "MiscellaneousDocumentMetadata")
            etree.SubElement(wrapper, "MiscellaneousDocumentType").text = "x"
            etree.SubElement(wrapper, "Language").text = "Spanish"
            doc = Doc(self.session, xml=etree.tostring(root))
            result = self.filter(doc, "name:DocTitle for Miscellaneous")
            self.assertFalse(result.error_log)
            self.assertFalse(result.messages)
            self.assertTrue(str(result.result_tree), "dada;x;Spanish")
        def test_017_organization(self):
            title = "National Cancer Institute;NCI;Bethesda;Maryland"
            test = "CdrCtl/Title eq {}".format(title)
            opts = dict(doctypes=["Organization"])
            results = Search(self.session, test, **opts).run()
            self.assertEqual(len(results), 1)
            self.assertTrue(isinstance(results[0], Doc))
            doc = results[0]
            result = self.filter(doc, "name:DocTitle for Organization")
            self.assertFalse(result.error_log)
            self.assertFalse(result.messages)
            self.assertEqual(str(result.result_tree), title)
        def test_018_outofscopepr(self):
            root = etree.Element("OutOfScopeProtocol")
            protocol_ids = etree.SubElement(root, "ProtocolIDs")
            wrapper = etree.SubElement(protocol_ids, "PrimaryID")
            etree.SubElement(wrapper, "IDString").text = "X"
            wrapper = etree.SubElement(protocol_ids, "OtherID")
            etree.SubElement(wrapper, "IDString").text = "Y"
            wrapper = etree.SubElement(root, "ProtocolTitle")
            etree.SubElement(wrapper, "TitleText").text = "Z" * 1024
            doc = Doc(self.session, xml=etree.tostring(root))
            result = self.filter(doc, "name:DocTitle for OutOfScopeProtocol")
            expected = ("X;Y;" + "Z" * 1024)[:255]
            self.assertFalse(result.error_log)
            self.assertFalse(result.messages)
            self.assertEqual(str(result.result_tree), expected)
        def test_019_pdqboardmemb(self):
            opts = dict(doctypes=["PDQBoardMemberInfo"], limit=1)
            title = "[No name found for board member]"
            test = "CdrCtl/Title ne {}".format(title)
            title = "!No title available for this document"
            tests = test, "CdrCtl/Title ne {}".format(title)
            results = Search(self.session, *tests, **opts).run()
            self.assertEqual(len(results), 1)
            self.assertTrue(isinstance(results[0], Doc))
            doc = results[0]
            result = self.filter(doc, "name:DocTitle for PDQBoardMemberInfo")
            expected = " (board membership information)"
            self.assertFalse(result.error_log)
            self.assertFalse(result.messages)
            self.assertEqual(str(result.result_tree), doc.title)
            self.assertTrue(str(result.result_tree).endswith(expected))
        """
        """

        def test_020_person_title(self):
            doc = self.get_doc("Person")
            result = self.filter(doc, "name:DocTitle for Person")
            self.assertFalse(result.error_log)
            self.assertFalse(result.messages)
            self.assertEqual(str(result.result_tree), doc.title)
        def test_021_politicalsub(self):
            doc = self.get_doc("PoliticalSubUnit")
            result = self.filter(doc, "name:DocTitle for PoliticalSubUnit")
            self.assertFalse(result.error_log)
            self.assertFalse(result.messages)
            self.assertEqual(str(result.result_tree), doc.title)
        def test_022_publishingsy(self):
            root = etree.Element("PublishingSystem")
            etree.SubElement(root, "SystemName").text = "Fake name for test"
            doc = Doc(self.session, xml=etree.tostring(root))
            result = self.filter(doc, "name:DocTitle for PublishingSystem")
            self.assertFalse(result.error_log)
            self.assertFalse(result.messages)
            self.assertEqual(str(result.result_tree), "Fake name for test")
        def test_023_scientificpr(self):
            root = etree.Element("ScientificProtocolInfo")
            protocol_ids = etree.SubElement(root, "ProtocolIDs")
            wrapper = etree.SubElement(protocol_ids, "PrimaryID")
            etree.SubElement(wrapper, "IDString").text = "A"
            wrapper = etree.SubElement(protocol_ids, "OtherID")
            etree.SubElement(wrapper, "IDString").text = "B"
            type_names = "Original", "Patient", "Professional"
            for i, type_name in enumerate(type_names):
                title = etree.SubElement(root, "ProtocolTitle")
                title.set("Type", type_name)
                title.text = "xyz"[i] * 1024
            doc = Doc(self.session, xml=etree.tostring(root))
            filter = "name:DocTitle for ScientificProtocolInfo"
            result = self.filter(doc, filter)
            expected = ("A;B;" + "x" * 1024)[:255]
            self.assertFalse(result.error_log)
            self.assertFalse(result.messages)
            self.assertEqual(str(result.result_tree), expected)
        def test_024_summary_titl(self):
            root = etree.Element("Summary", ModuleOnly="Yes")
            etree.SubElement(root, "SummaryTitle").text = "gimte"
            wrapper = etree.SubElement(root, "SummaryMetaData")
            etree.SubElement(wrapper, "SummaryType").text = "pretend"
            audience = "Fernsehzuschauer"
            etree.SubElement(wrapper, "SummaryAudience").text = audience
            expected = "gimte [Module];pretend;Fernsehzuschauer"
            doc = Doc(self.session, xml=etree.tostring(root))
            result = self.filter(doc, "name:DocTitle for Summary")
            self.assertFalse(result.error_log)
            self.assertFalse(result.messages)
            self.assertEqual(str(result.result_tree), expected)
        def test_025_supplementar(self):
            root = etree.Element("SupplementaryInfo")
            etree.SubElement(root, "Title").text = "extra"
            etree.SubElement(root, "Category").text = "cheese"
            doc = Doc(self.session, xml=etree.tostring(root))
            result = self.filter(doc, "name:DocTitle for SupplementaryInfo")
            self.assertFalse(result.error_log)
            self.assertFalse(result.messages)
            self.assertEqual(str(result.result_tree), "extra;cheese")
        def test_026_term_title__(self):
            doc = self.get_doc("Term")
            result = self.filter(doc, "name:DocTitle for Term")
            self.assertFalse(result.error_log)
            self.assertFalse(result.messages)
            self.assertEqual(str(result.result_tree), doc.title)
        def test_027_termset_titl(self):
            root = etree.Element("TermSet")
            etree.SubElement(root, "TermSetName").text = "dada"
            etree.SubElement(root, "TermSetType").text = "gimte"
            doc = Doc(self.session, xml=etree.tostring(root))
            result = self.filter(doc, "name:DocTitle for TermSet")
            self.assertFalse(result.error_log)
            self.assertFalse(result.messages)
            self.assertEqual(str(result.result_tree), "dada;gimte")


# Put new tests up here while you're working on them, then move them
# above when they're ready.
if True:
    class _02VendorFilterSet(Tests):
        """ """
        def test_028_vendor_ctgov(self):
            self.assertFalse("vendor ctgovprotocol set not implemented")
        def test_029_vendor_count(self):
            self.assertFalse("vendor country set not implemented")
        def test_030_vendor_drugi(self):
            self.assertFalse("vendor druginfosummary set not implemented")
        def test_031_vendor_genet(self):
            self.assertFalse("vendor geneticsprofessional set not implemented")
        def test_032_vendor_gloss(self):
            self.assertFalse("vendor glossaryterm set not implemented")
        def test_033_vendor_insco(self):
            self.assertFalse("vendor inscopeprotocol set not implemented")
        def test_034_vendor_organ(self):
            self.assertFalse("vendor organization set not implemented")
        def test_035_vendor_perso(self):
            self.assertFalse("vendor person set not implemented")
        def test_036_vendor_polit(self):
            self.assertFalse("vendor politicalsubunit set not implemented")
        """ """
        def test_037_vendor_summa(self):
            doc = Doc(self.session, id=62902, version="lastp")
            result = self.filter(doc, "set:Vendor Summary Set")
            expected = "<SummaryType>Treatment</SummaryType>"
            self.assertTrue(expected in str(result.result_tree))
            self.assertFalse(result.error_log)
            expected = "ALT attribute of MediaLink element is empty."
            self.assertTrue(expected in repr(result.messages))
        def test_038_vendor_term_(self):
            doc = self.get_published_doc("Term")
            result = self.filter(doc, "set:Vendor Term Set")
            self.assertFalse(result.error_log)
            self.assertFalse(result.messages)
            self.assertIn("<PreferredName>", str(result.result_tree))
            #print(self.get_published_xml(doc.id))
            #print(result.result_tree)
            #print(result.error_log)
            #print(result.messages)
        def test_999_check_covera(self):
            docs = FilterSet.get_filters(self.session)
            filters = set([doc.title for doc in docs])
            uncovered = filters - Tests.COVERED
            with open("covered", "wb") as fp:
                for name in sorted(Tests.COVERED):
                    fp.write(name.encode("utf-8"))
                    fp.write(b"\n")
            with open("not_covered", "wb") as fp:
                for name in sorted(uncovered):
                    fp.write(name.encode("utf-8"))
                    fp.write(b"\n")
            args = len(Tests.COVERED), len(filters)
            message = "only {} of {} filters covered".format(*args)
            assert not uncovered, message

if __name__ == "__main__":
    unittest.main()
