#----------------------------------------------------------------------
# Preprocess login creation/configuration script for this tier.
#----------------------------------------------------------------------
import cdrutil, cdrpw, time

script = open("CreateLogins.sql", "rb").read()
env = cdrutil.getEnvironment()
tier = cdrutil.getTier()
dboPw = cdrpw.password(env, tier, "cdr", "CdrSqlAccount")
guestPw = cdrpw.password(env, tier, "cdr", "CdrGuest")
pubPw = cdrpw.password(env, tier, "cdr", "CdrPublishing")
script = script.replace("@@DBOPW@@", dboPw)
script = script.replace("@@GUESTPW@@", guestPw)
script = script.replace("@@PUBPW@@", pubPw)
now = time.strftime("%Y%m%d%H%M%S")
name = "CreateLogins-%s.sql" % now
fp = open(name, "wb")
fp.write(script)
fp.close()
print "wrote", name
