package gov.epa.surrogate.qa;

public class Threshold {

	private double thresh;
	private double precision=1e-5;

	public Threshold() {
		thresh = 0.5;
	}

	public Threshold(double thresh) {
		this.thresh = thresh;
	}

	public boolean isTotalBig(double value) {
		return ( (value - thresh) >= precision);
	}
	
	public double getThreshold() {
		return thresh;
	}

}
