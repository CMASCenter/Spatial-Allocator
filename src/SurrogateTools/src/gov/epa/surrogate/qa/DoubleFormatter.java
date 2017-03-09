package gov.epa.surrogate.qa;

import java.text.DecimalFormat;

public class DoubleFormatter {

	private DecimalFormat format;

	public DoubleFormatter() {
		format = new DecimalFormat("0.00000000");
	}

	public String format(double value) {
		return format.format(value);
	}

}
