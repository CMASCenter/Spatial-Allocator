package gov.epa.surrogate.gapfill;

import gov.epa.surrogate.SystemInfo;

public class Main {

	public static void main(String[] args) {
		if (args.length < 1) {
			System.out.println("Usage: java gov.epa.surrogate.gapfill.Main gapfillInput.txt");
			System.exit(1);
		}
		new SystemInfo().print();
		try {
			Gapfilling gf = new Gapfilling(args[0]);
			gf.doGapfill();
		} catch (Exception e) {
			System.out.println(e.getMessage());
			System.exit(1);
		}
		
	}

}
