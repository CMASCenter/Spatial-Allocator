package gov.epa.surrogate.normalize;

import java.text.DecimalFormat;

import gov.epa.surrogate.qa.DoubleFormatter;

public class NormalizeSurrogateRow {

	private int surrogateID;

	private int countyID;

	private double ratio;

	private String comment;

	private long gridX;

	private long gridY;

	private String delimiter;

	private DoubleFormatter formatter;
	
	private int minimumTokens;
	
	private Boolean polyInCty=true;

	public NormalizeSurrogateRow(String[] tokens, int minimumTokens, String delimiter) {
		this.minimumTokens = minimumTokens;
		surrogateID = new Integer(tokens[0]).intValue();
		countyID = new Integer(tokens[1]).intValue();
		gridX = new Long(tokens[2]).longValue();
		if ( minimumTokens == 5)
			gridY = new Long(tokens[3]).longValue();
		else {
			//When gridX is census track
			//Start index is inclusive; End index is exclusive
			//remove the lines with inconsistent county id
			int xCounty = new Integer(tokens[2].substring(0, tokens[2].length()-6)).intValue();
			if ( countyID != xCounty )
				polyInCty = false;
			gridY = -1;
		}
		ratio = new Double(tokens[minimumTokens - 1]).doubleValue();
		if (tokens.length == minimumTokens + 1) {
			comment = tokens[minimumTokens];
		}
		this.delimiter = delimiter;
		this.formatter = new DoubleFormatter();
	}

	public String getComment() {
		return comment;
	}

	public int getCountyID() {
		return countyID;
	}

	public double getRatio() {
		return ratio;
	}

	public int getSurrogateID() {
		return surrogateID;
	}

	public String getLine() {
		StringBuffer sb = new StringBuffer();
		// TODO: formatting
		DecimalFormat I5 = new DecimalFormat("00000");	
		DecimalFormat d8 = new DecimalFormat("0.00000000");	
		DecimalFormat I11 = new DecimalFormat("00000000000");	 
		sb.append(surrogateID + delimiter);
		sb.append(I5.format(countyID) + delimiter);
		if ( minimumTokens == 4 ) sb.append(I11.format(gridX) + delimiter);
		if ( minimumTokens == 5 ) {
			sb.append(gridX + delimiter);
			sb.append(gridY + delimiter);
		}
		sb.append(d8.format(ratio) + delimiter);
		sb.append("!" + delimiter + comment);
		return sb.toString();
	}
	
	public Boolean getPolyInCty() {
		return polyInCty;
	}

	protected String format(double value) {
		return formatter.format(value);
	}

	public void setRatio(double ratio) {
		this.ratio = ratio;
	}

}
