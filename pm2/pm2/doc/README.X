  Lorsque ssh forwarde X11, il utilise le programme xauth pour
enregistrer le nouveau DISPLAY à utiliser.
  Lors de multiples connexions simultanées par ssh avec forwarding de
X11 sur des machines avec le même compte $HOME (partagé par NFS par
exemple), les "xauth" sont fait en parallèles et essaye d'accéder tous
en même temps au fichier $HOME/.Xauthority.

  Pour résoudre ce problème, en attendant que xauth soit corrigé, il est
possible d'utiliser un fichier Xauthority différent pour chaque
connection ssh, ce qui évite les problèmes d'accès concurrent.

  Pour cela :
rajouter le script ${HOME}/.ssh/rc contenant :
----------------------------------------------------
#!/bin/sh

if [ -n "$SSH_AUTH_SOCK" ]; then
        DIRNAME="$(dirname $SSH_AUTH_SOCK)"
	export XAUTHORITY="$DIRNAME/Xauthority"
	mkdir -p `dirname "$DIRNAME"`
fi
#echo XAUTHORITY=$XAUTHORITY
if read proto cookie && [ -n "$DISPLAY" ]; then
	if [ -n "$XAUTHORITY" -a ! -f "$XAUTHORITY" ] ; then
		touch "$XAUTHORITY"
	fi
	if [ `echo $DISPLAY | cut -c1-10` = 'localhost:' ]; then
		# X11UseLocalhost=yes
		echo add unix:`echo $DISPLAY |
		cut -c11-` $proto $cookie
	else
		# X11UseLocalhost=no
		echo add $DISPLAY $proto $cookie
	fi | xauth -q -
fi
----------------------------------------------------

Puis, si vous utilisez bash:
****************************
Rajoutez dans ${HOME}/.bashrc (et ${HOME}/.bash_profile s'il n'inclut pas
${HOME}/.bashrc) :
----------------------------------------------------
if [ -n "$SSH_AUTH_SOCK" ]; then
	DIRNAME="$(dirname $SSH_AUTH_SOCK)"
	if [ -f "$DIRNAME/Xauthority" ]; then
		export XAUTHORITY="$DIRNAME/Xauthority"
	fi
fi
----------------------------------------------------

Si vous utilisez [t]csh:
****************************
Rajoutez dans ${HOME}/.cshrc (ou ${HOME}/.tcshrc si nécessaire)
----------------------------------------------------
if ( $?SSH_AUTH_SOCK ) then
	DIRNAME="`dirname $SSH_AUTH_SOCK`"
	if ( -f "$DIRNAME/Xauthority" ) then
		setenv XAUTHORITY "$DIRNAME/Xauthority"
	endif
endif




