package gov.epa.surrogate;

public class Precision {

	private double precision;

	public Precision() {
		precision = 1e-5;
	}

	public Precision(double precision) {
		this.precision = precision;
	}

	public boolean isTotalOne(double value) {
		return (Math.abs(value - 1.0) <= precision);
	}

}
