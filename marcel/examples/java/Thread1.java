//utilisation de threads

import java.io.*;
import java.util.*;

public class Thread1 {
	 public static void main(String[] arg) throws Exception {
		  // init thread courant
		  Thread main=Thread.currentThread();
		  // affichage
		  System.out.println("Thread courant : " + main.getName());
		  // on change le nom
		  main.setName("myMainThread");
		  // vérification
		  System.out.println("Thread courant : " + main.getName());

		  // boucle infinie
		  while(true) {
				// on récupère l'heure
				Calendar calendrier=Calendar.getInstance();
				String H=calendrier.get(Calendar.HOUR_OF_DAY)+":"+calendrier.get(Calendar.MINUTE)+":"+calendrier.get(Calendar.SECOND);
				// affichage
				System.out.println(main.getName() + " : " +H);
				// arrêt temporaire
				Thread.sleep(1000);
		  }// while
	 }// main
}// classe
		  
																																							  
