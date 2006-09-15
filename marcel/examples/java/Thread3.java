//utilisation de threads

import java.io.*;
import java.util.*;

public class Thread3 {
	 public static void main(String[] arg) {
		  // init thread courant
		  Thread main = Thread.currentThread();
		  // on donne un nom au thread courant
		  main.setName("myMainThread");
		  // début de main
		  System.out.println("début du thread " +main.getName());

		  //création de threads d'exécution
		  Thread[] taches=new Thread[5];
		  for (int i=0;i<taches.length;i++) {
				// on crée le thread i
				taches[i]=new Thread() {
					 public void run() {
						  affiche();
					 }
				};// def taches[i]
				// on fixe le nom du thread i
				taches[i].setName(""+i);
				// on lance l'exécution du thread i
				taches[i].start();
		  }// for
	 
		  // attente de tous les threads
		  try {
				for (int i=0;i<taches.length;i++) {
					 // on attend le thread i
					 taches[i].join();
				}// for
		  } catch (Exception ex){
				System.out.println("exception autour de join");
		  }
		  // fin de main
		  System.out.println("fin du thread " + main.getName());        // arret de l'application
		  System.exit(0);
	 }// main
	 
	 public static void affiche() {
		  // on recupere l'heure
		  Calendar calendrier=Calendar.getInstance();
		  String H=calendrier.get(Calendar.HOUR_OF_DAY)+":"+calendrier.get(Calendar.MINUTE)+":"+calendrier.get(Calendar.SECOND);
		  //affichage debut d'exécution
		  System.out.println("Debut d'execution de la methode affiche dans le Thread "+ Thread.currentThread().getName()+ " : " + H);
		  //mise en sommeil pendant 1 s
		  try {
				Thread.sleep(1000);
		  } catch (Exception ex){}
		  // on recupere l'heure
		  calendrier=Calendar.getInstance();
		  H=calendrier.get(Calendar.HOUR_OF_DAY)+":"+calendrier.get(Calendar.MINUTE)+":"+calendrier.get(Calendar.SECOND);
		  //affichage fin d'execution
		  System.out.println("Fin d'execution de la methode affiche dans le thread "+ Thread.currentThread().getName()+ " : " + H);
	 }//affiche
}
