#----------------------------------------------------------------------
# Preprocess login creation/configuration script for this tier.
#----------------------------------------------------------------------
import time
import cdrpw
from cdrapi.settings import Tier

script = open("CreateLogins.sql", "rb").read()
try:
    with open("d:/etc/cdrenv.rc") as fp:
        env = fp.read().strip()
except:
    env = "CBIIT"
tier = Tier().name
dboPw = cdrpw.password(env, tier, "cdr", "CdrSqlAccount")
guestPw = cdrpw.password(env, tier, "cdr", "CdrGuest")
pubPw = cdrpw.password(env, tier, "cdr", "CdrPublishing")
script = script.replace("@@DBOPW@@", dboPw)
script = script.replace("@@GUESTPW@@", guestPw)
script = script.replace("@@PUBPW@@", pubPw)
now = time.strftime("%Y%m%d%H%M%S")
name = "CreateLogins-%s.sql" % now
with open(name, "wb") as fp:
    fp.write(script)
print("wrote", name)
