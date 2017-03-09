package gov.epa.surrogate.merge;

public class Equation {

	private double f1;

	private double f2;
	
	public Equation(double factor1) {
		this.f1 = factor1;
		this.f2 = 0.0;
	}

	public Equation(double factor1, double factor2) {
		this.f1 = factor1;
		this.f2 = factor2;
	}

	public double evaluate(double v1, double v2) {
		return f1 * v1 + f2 * v2;
	}
	
	public double evaluate(double v1) {
		return f1 * v1;
	}
	
	public double getf1(){
		return f1;
	}
	
	public double getf2(){
		return f2;
	}
}
