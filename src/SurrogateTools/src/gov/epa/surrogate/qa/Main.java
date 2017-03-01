package gov.epa.surrogate.qa;

import gov.epa.surrogate.SystemInfo;

public class Main {
	
	public static void main(String[] arguments) {
		
		if (arguments.length < 1) {
			System.out.println("Usage: java gov.epa.surrogate.qa.Main SRGDESC.txt");
			System.exit(1);
		}
		new SystemInfo().print();
		try {
			QAReports ss = new QAReports(arguments[0]);
			double thresh = Double.parseDouble(arguments[1]);
			Threshold tt = new Threshold(thresh);
			ss.execute(tt);
		} catch (Exception e) {
			System.out.println(e.getMessage());
			System.exit(1);
		}
		
	}

}
