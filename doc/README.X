  Lorsque ssh forwarde X11, il utilise le programme xauth pour
enregistrer le nouveau DISPLAY à utiliser.
  Lors de multiples connexions simultanées par ssh avec forwarding de
X11 sur des machines avec le même compte $HOME (partagé par NFS par
exemple), les "xauth" sont fait en parallèles et essaye d'accéder tous
en même temps au fichier $HOME/.Xauthority.

  Pour résoudre ce problème, en attendant que xauth soit corrigé, il
existe deux solutions :

-1- sérialiser les appels à ssh

-2- utiliser un fichier Xauthority différent pour chaque connection
ssh, ce qui évite les problèmes d'accès concurrent.



Solution 1 (Alexandre DENIS)
----------

-- positionner LEO_RSH à "ssh -f" ('-f' demande à ssh de forker dès
que la connexion est établie)
-- lancer l'application avec "leonie -w" ('-w' demande à leonie
d'attendre que le lanceur forke quand il est prêt).

L'impact sur la vitesse de lancement n'est pas flagrant.




Solution 2 (Vincent DANJEAN)
----------

  Pour cela :
rajouter le script ${HOME}/.ssh/rc contenant :
----------------------------------------------------
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
----------------------------------------------------

Puis, si vous utilisez bash:
****************************
Rajoutez dans ${HOME}/.bashrc (et ${HOME}/.bash_profile s'il n'inclut pas
${HOME}/.bashrc) :
----------------------------------------------------
if [ -n "$DISPLAY" ];
then
	suf=`echo $DISPLAY | cut -c11-`
	export XAUTHORITY="/tmp/.${USER}_xa$suf"
fi
----------------------------------------------------

Si vous utilisez [t]csh:
****************************
Rajoutez dans ${HOME}/.cshrc (ou ${HOME}/.tcshrc si nécessaire)
----------------------------------------------------
if ( $?DISPLAY ) then
	suf="`echo $DISPLAY | cut -c11-`"
	setenv XAUTHORITY "/tmp/.${USER}_xa$suf"
endif




