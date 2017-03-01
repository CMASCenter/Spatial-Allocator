package gov.epa.surrogate.normalize;

import gov.epa.surrogate.SystemInfo;

public class Main {

	public static void main(String[] arguments) {

		int length = arguments.length;
		if (length < 1 || length > 3) {
			System.out.println("Usage: java gov.epa.surrogate.normalize.Main SRGDESC.txt [ExcludeCounties.txt] [1e-6]");
			System.exit(1);
		}
		
		new SystemInfo().print();
		try {
			NormalizeSurrogates ns = null;
			if (length == 3) {
				ns = new NormalizeSurrogates(arguments[0], arguments[1], arguments[2]);
			}
			if (length == 2) {
				ns = new NormalizeSurrogates(arguments[0], arguments[1]);
			}
			if (length == 1) {
				ns = new NormalizeSurrogates(arguments[0]);
			}
			ns.normalize();
		} catch (Exception e) {
			System.out.println(e.getMessage());
			System.exit(1);
		}

	}

}
