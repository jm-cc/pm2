public class Testthreads {
	public static void main(String[] argv) {
		Affiche t1=new Affiche("hello");
		Affiche t2=new Affiche("kongnitchiwa");
		t1.start();
		System.out.println("blip");
		t2.start();
		t2.setPriority(6);
		t1.setPriority(6);
		System.out.println("blop");
	}
}
