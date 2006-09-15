import java.io.*;
import javax.swing.*;
import java.util.*;

public class Thread4 {

	 //contruire le cadre
	 
	 public static void main(String arg[]) {
		  BufferedReader fluxEntree = null;
		  String ch = "";
		  double x;
		  Horloge h = new Horloge();
		  while(true){
				try {
					 

					 fluxEntree = new BufferedReader(new InputStreamReader(System.in));
			ch = fluxEntree.readLine();
			x = Double.valueOf(ch).doubleValue();
			System.out.println("Double lu : "+x);
//attendre entree
			
h.lire();
				}
				catch(Exception e) {
					 
				}
		  }
	 }
}

