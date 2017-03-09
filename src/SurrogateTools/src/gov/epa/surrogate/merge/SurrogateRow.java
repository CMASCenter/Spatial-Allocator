package gov.epa.surrogate.merge;

import java.text.DecimalFormat;

public class SurrogateRow implements Comparable {

	private int surrogateCode;

	private double ratio;

	private int countyCode;

	private String delimiter;

	private long row;

	private long column;

	private String comment;

	public SurrogateRow() {
		delimiter = "\t";
		surrogateCode = -1;
		countyCode = -1;
		row = -1;
		column = -1;
		comment = null;
	}

	public SurrogateRow(String line, int minTokens) {
		this();
		String[] tokens = parse(line, delimiter);
		surrogateCode = new Integer(tokens[0].trim()).intValue();
		countyCode = new Integer(tokens[1].trim()).intValue();
		row = new Long(tokens[2].trim()).longValue();
		column = column(tokens, minTokens);		
		ratio = new Double(tokens[minTokens - 1]).doubleValue();
		if (tokens.length == minTokens + 2) {
			comment = tokens[minTokens + 1];
		}
	}

	private long column(String[] tokens, int minTokens) {
		return (minTokens == 5) ? new Long(tokens[minTokens - 2].trim()).intValue() : -1;
	}

	private String[] parse(String line, String delimiter) {
		return line.split(delimiter);
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
	
	public String getComment(){
		return comment;
	}

	/*
	 * this>other => +1 this==other => 0 this<other => -1
	 */
	public int compareTo(Object o) {
		if (o == null) {
			return 1;
		}

		SurrogateRow surrogateRow = (SurrogateRow) o;
//		if (surrogateCode > surrogateRow.getSurrogateCode()) {
//			return 1;
//		}
//		if (surrogateCode < surrogateRow.getSurrogateCode()) {
//			return -1;
//		}

		if (countyCode > surrogateRow.getCountyCode()) {
			return 1;
		}
		if (countyCode < surrogateRow.getCountyCode()) {
			return -1;
		}

		if (row > surrogateRow.getRow()) {
			return 1;
		}
		if (row < surrogateRow.getRow()) {
			return -1;
		}

		if (column > surrogateRow.getColumn()) {
			return 1;
		}

		if (column < surrogateRow.getColumn()) {
			return -1;
		}

		return 0; // srgId, countyId, row, columns are equal
	}

	public long getColumn() {
		return column;
	}

	public long getRow() {
		return row;
	}

	public String getLine() {
		DecimalFormat I5 = new DecimalFormat("00000");	
		DecimalFormat d8 = new DecimalFormat("0.00000000");	
		DecimalFormat I11 = new DecimalFormat("00000000000");	 
		 
		return surrogateCode + delimiter + I5.format(countyCode) + delimiter  
				+ ((column == -1) ? I11.format(row) + delimiter : row + delimiter)
				+ ((column == -1) ? "" : (column + delimiter)) + d8.format(ratio) + delimiter
				+ ((comment == null || comment.trim().isEmpty() ) ? "" : delimiter + "!" + delimiter + comment);
	}

	public void setSurrogateCode(int surrogateID) {
		this.surrogateCode = surrogateID;
	}

	public void setCountyCode(int countyCode) {
		this.countyCode = countyCode;
	}

	public void setRow(long row) {
		this.row = row;
	}

	public void setColumn(long column) {
		this.column = column;
	}

	public void setRatio(double ratio) {
		this.ratio = ratio;
	}

	public void setComment(String comment) {
		this.comment = comment;
	}
}
