/* REXX script to get the svn revision and display it. */
Trace o
Address System 'svnversion . | rxqueue > nul:'
/* default version to 0, if svnversion doesn't exist or no .svn here */
ver = 0
If rc = 0 Then
   Do
      /* Using PARSE PULL preserves case */
      /* If it is a double value get the first value only */
      Parse Pull sval ver ':' .
      If Strip( ver ) = '' Then ver = sval
      If ver = 'exported' Then ver = 0
      If Datatype( Right( ver, 1 ) ) \= 'NUM' Then ver = Substr( ver, 1, Length( ver) - 1 )
   End
Say ver
Exit 0
