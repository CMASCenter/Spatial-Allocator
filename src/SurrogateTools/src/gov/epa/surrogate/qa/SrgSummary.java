package gov.epa.surrogate.qa;

import gov.epa.surrogate.Precision;
import gov.epa.surrogate.Surrogates;

import java.io.PrintWriter;

public class SrgSummary {

	private PrintWriter writer;

	private String header;

	private Surrogates surrogates;

	private String delimiter;

	private DoubleFormatter formatter;

	private Precision precision;
	private Threshold thresh;

	public SrgSummary(PrintWriter writer, String delimiter, Counties counties, String header, Surrogates surrogates, Threshold thresh)
			throws Exception {
		this.writer = writer;
		this.delimiter = delimiter;
		checkForEmptyCounty(counties);
		this.header = header;
		this.surrogates = surrogates;
		formatter = new DoubleFormatter();
		this.precision = new Precision();
		this.thresh = thresh;
	}

	private void checkForEmptyCounty(Counties counties) throws Exception {
		if (counties.isEmpty()) {
			throw new Exception("There is no data");
		}
	}

	protected void printHeader() {
		writer.println(header);
	}

	protected void printColumnHeader(int[] surrogateCodes) throws Exception {
		// 1st row of column header
		if (thresh != null )
			printValueWithLineSeparator("Threshold: " +  thresh.getThreshold());
		// 2nd row of column header
		printValueWithDelimiter("COUNTY");
		
		for (int i = 0; i < surrogateCodes.length - 1; i++) {
			printValueWithDelimiter(surrogateCodes[i]);
		}
		printValueWithLineSeparator("" + surrogateCodes[surrogateCodes.length - 1]);
		// 3nd row of column header
		printValueWithDelimiter("");
		for (int i = 0; i < surrogateCodes.length - 1; i++) {
			printValueWithDelimiter("\"" + surrogates.getSurrogateName(surrogateCodes[i]) + "\"");
		}
		printValueWithLineSeparator("\"" + surrogates.getSurrogateName(surrogateCodes[surrogateCodes.length - 1])
				+ "\"");
	}

	
	protected void printHoldColumnHeader(int[] surrogateCodes, int minTokens) throws Exception {
		// 1st row of column header
		if (thresh != null )
			printValueWithLineSeparator("Threshold: " +  thresh.getThreshold());
		// 2nd row of column header
		printValueWithDelimiter("COUNTY");
		if ( minTokens == 5 ) 
			printValueWithDelimiter("COL,ROW");
		else
			printValueWithDelimiter("POLYID");
			
		for (int i = 0; i < surrogateCodes.length - 1; i++) {
			printValueWithDelimiter(surrogateCodes[i]);
		}
		printValueWithLineSeparator("" + surrogateCodes[surrogateCodes.length - 1]);
		// 3nd row of column header
		printValueWithDelimiter("");
		printValueWithDelimiter("");
		if (minTokens == 5 )
			printValueWithDelimiter("");
		for (int i = 0; i < surrogateCodes.length - 1; i++) {
			printValueWithDelimiter("\"" + surrogates.getSurrogateName(surrogateCodes[i]) + "\"");
		}
		printValueWithLineSeparator("\"" + surrogates.getSurrogateName(surrogateCodes[surrogateCodes.length - 1])
				+ "\"");
	}
	protected void printValueWithDelimiter(int value) {
		writer.print(value + delimiter);
	}
	
	protected void printValueWithDelimiter(long value) {
		writer.print(value + delimiter);
	}

	protected void printValueWithDelimiter(String value) {
		writer.print(value + delimiter);
	}

	protected void printValueWithLineSeparator(String value) {
		writer.println(value); // println adds a appropriate line separator based on OS
	}

	protected String format(double sum) {
		return formatter.format(sum);
	}

	protected boolean isTotalOne(double sum) {
		return precision.isTotalOne(sum);
	}
	
	protected boolean isTotalBig(double sum) {
		return thresh.isTotalBig(sum);
	}

	protected void close() {
		writer.close();
	}
}
