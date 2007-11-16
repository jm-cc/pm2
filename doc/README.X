When SSH forwards X11, it uses xauth to set the new DISPLAY to be
used. When starting multiple connections on machines sharing a $HOME
directory, the different xauth being started in parallel, will all try
to access at the same time to the file $HOME/.Xauthority.

Two solutions are available to solve this problem:

-1- serialise the calls to ssh.

-2- use a different Xauthority file for each connection.



Solution 1 (Alexandre DENIS)
----------

-- Set LEO_RSH to "ssh -f" ('-f' requests ssh to fork as soon as the
   connection is established).
-- Start the application using "leonie -w" ('-w' requests leonie to
   wait for the loader to fork).

This solution does not have any noticeable effect on the speed of the
loading process.


Solution 2 (Vincent DANJEAN)
----------

-- Create the script ${HOME}/.ssh/rc :

----xx--CUT HERE--xx-------------------------------------------
#!/bin/sh

suf=`echo $DISPLAY | cut -c11-`
if read proto cookie && [ -n "$DISPLAY" ]; then
	if [ `echo $DISPLAY | cut -c1-10` = 'localhost:' ]; then
		# X11UseLocalhost=yes
		echo add unix:$suf $proto $cookie
	else
		# X11UseLocalhost=no
		echo add $DISPLAY $proto $cookie
	fi | xauth -f "/tmp/.${USER}_xa$suf" -q -
fi
----xx--CUT HERE--xx-------------------------------------------

-- For bash's users
  -- Add the following lines in ${HOME}/.bashrc (and in
     ${HOME}/.bash_profile unless it includes ${HOME}/.bashrc)

----xx--CUT HERE--xx-------------------------------------------
if [ -n "$DISPLAY" ];
then
	suf=`echo $DISPLAY | cut -c11-`
	export XAUTHORITY="/tmp/.${USER}_xa$suf"
fi
----xx--CUT HERE--xx-------------------------------------------

-- For [t]csh's users:
  -- Add the following lines in ${HOME}/.cshrc (or ${HOME}/.tcshrc)

----xx--CUT HERE--xx-------------------------------------------
if ( $?DISPLAY ) then
	suf="`echo $DISPLAY | cut -c11-`"
	setenv XAUTHORITY "/tmp/.${USER}_xa$suf"
endif
----xx--CUT HERE--xx-------------------------------------------
