import sys

class Record:
    stamp = len(sys.argv) > 1 and sys.argv[1] or "20160416094024"
    def __init__(self, names, vals):
        for i, name in enumerate(names):
            setattr(self, name, vals[i])
        self.primary_key = vals[0]
    @classmethod
    def load(cls, table):
        filename = "%s-%s.txt" % (table, cls.stamp)
        lines = open(filename).readlines()
        names = eval(lines[0].strip())
        records = [cls(names, eval(line.strip())) for line in lines[1:]]
        return dict([(record.primary_key, record) for record in records])

class Records:
    products = Record.load("data_product")
    orgs = Record.load("data_partner_org")
    contacts = Record.load("data_partner_contact")
    @staticmethod
    def value(v, trim_date=False):
        if v is None:
            return ""
        v = str(v)
        if trim_date and len(v) > 10:
            v = v[:10]
        return v
    @classmethod
    def dump(cls):
        for contact in cls.contacts.values():
            org = cls.orgs[contact.org_id]
            product = cls.products[org.prod_id]
            values = [
                cls.value(product.prod_name),
                "Y",
                cls.value(contact.email_addr),
                cls.value(contact.person_name),
                cls.value(org.org_name),
                cls.value(contact.phone),
                cls.value(org.org_status),
                cls.value(contact.notif_count),
                cls.value(contact.contact_type),
                cls.value(org.ftp_username),
                "",
                cls.value(org.org_id),
                cls.value(org.activated, trim_date=True),
                cls.value(org.terminated, trim_date=True),
                cls.value(org.renewal, trim_date=True),
                cls.value(contact.notif_date)
            ]
            print ":".join(values)
if __name__ == "__main__":
    Records.dump()
