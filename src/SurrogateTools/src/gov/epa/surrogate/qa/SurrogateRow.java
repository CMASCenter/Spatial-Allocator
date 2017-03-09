package gov.epa.surrogate.qa;

public class SurrogateRow {

	private int surrogateCode;

	private double ratio;

	private int gapFillCode;

	private boolean gapFilled;

	private int countyCode;
	
	private long row=-1;

	private long column=-1;

	public SurrogateRow(String[] tokens, int minTokens, boolean gapFilled) {
		// FIXME: validation
		surrogateCode = new Integer(tokens[0].trim()).intValue();
		countyCode = new Integer(tokens[1].trim()).intValue();
		row = new Long(tokens[2].trim()).longValue();
		column = column(tokens, minTokens);	
		ratio = new Double(tokens[minTokens - 1]).doubleValue();
		this.gapFilled = gapFilled;
		gapFillCode = gapFillCode(tokens, gapFilled);

	}
	
	private long column(String[] tokens, int minTokens) {
		return (minTokens == 5) ? new Long(tokens[minTokens - 2].trim()).intValue() : -1;
	}

	private int gapFillCode(String[] tokens, boolean gapFilled) {
		if (!gapFilled) {
			return -1;
		}
		String lastToken = tokens[tokens.length - 1];
		if(!lastToken.contains("GF:")){
			this.gapFilled = false;
			return -1;
		}
		String[] gfTokens = lastToken.split(":");
		return Integer.parseInt(gfTokens[1].trim());

	}

	public int getSurrogateCode() {
		return surrogateCode;
	}

	public double getRatio() {
		return ratio;
	}

	public int getCountyCode() {
		return countyCode;
	}

	public int getGapFillCode() {
		return gapFillCode;
	}

	public boolean isGapFilled() {
		return gapFilled;
	}
	
	public long getColumn() {
		return column;
	}

	public long getRow() {
		return row;
	}

}
