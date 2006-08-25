import java.util.*;
public class Affiche extends Thread {
	String mess;
	public Affiche(String mess) {
		this.mess=mess;
	}
	public void run() {
		int i;
		Random blip=new Random();
		for (i=0;i<10;i++) {
			System.out.println(mess);
			try {
				//sleep(blip.nextInt(1001));
				int j;
				for (j=0;j<100000000;j++);
				yield();
			} catch (Exception e) {};
		}
	}
}
