SELECT '<CdrCommandSet><SessionId>' + name + '</SessionId><CdrCommand><CdrLogoff/></CdrCommand></CdrCommandSet>'
  FROM session
 WHERE ended IS NULL
