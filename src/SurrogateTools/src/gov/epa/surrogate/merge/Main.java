package gov.epa.surrogate.merge;

import gov.epa.surrogate.SystemInfo;

public class Main {

	public static void main(String[] args) {
		if (args.length < 1) {
			System.out.println("Usage: java gov.epa.surrogate.merge.Main mergeInput.txt");
			System.exit(1);
		}
		new SystemInfo().print();
		try {
			Merging mg = new Merging(args[0]);
			mg.doMerging();
		} catch (Exception e) {
			System.out.println(e.getMessage());
			System.exit(1);
		}
		
	}

}
