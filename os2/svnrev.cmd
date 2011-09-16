/* REXX script to get the svn revision and save it to SVN.REV. */
"svnversion -n . | rxqueue"
/* Using PARSE PULL preserves case */
Parse Pull sval
/* If it is a double value get the first value */
Parse Var sval ver ':' newver 'M' 'S'
/* Remove trailing M or S from the version */
Parse Var ver tmpver 'M'
Parse Var tmpver ver 'S'
err = LINEOUT('SVN.REV', 'VERREV=' || ver, 1)
/* Check for an error. */
IF err \== 0 THEN DO
Bad:
   SAY "ERROR:" STREAM('SVN.REV', 'D')
   RETURN
END
