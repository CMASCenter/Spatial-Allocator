package gov.epa.surrogate.qa;

import gov.epa.surrogate.Surrogates;

import java.io.File;
import java.io.FileWriter;
import java.io.PrintWriter;

public class SurrogateNot1Report {

	private Counties counties;

	private String delimiter;

	private String fileName;

	private SrgSummary delegate;

	public SurrogateNot1Report(String fileName, Counties counties, String header, Surrogates surrogates)
			throws Exception {
		this.fileName = fileName;
		this.delimiter = ",";
		this.counties = counties;
		PrintWriter writer = new PrintWriter(new FileWriter(new File(fileName)));
		this.delegate = new SrgSummary(writer, delimiter, counties, header, surrogates, null);
	}

	public void write() throws Exception {
		System.out.println("The output surrogate NOT1 file: " + fileName);

		int[] countyCodes = counties.allCountyCodes();
		int[] surrogateCodes = counties.allSurrgateCodes();
		int noOfSurrogates = surrogateCodes.length;
		delegate.printHeader();
		delegate.printColumnHeader(surrogateCodes);
		for (int i = 0; i < countyCodes.length; i++) {
			County county = counties.getCounty(countyCodes[i]);
			if (atLeastOneNot1InARow(county, surrogateCodes)) {
				delegate.printValueWithDelimiter(county.getCountyCode());
				for (int j = 0; j < noOfSurrogates - 1; j++) {
					CountySurrogate surrogate = county.surrogate(surrogateCodes[j]);
					delegate.printValueWithDelimiter(value(surrogate));
				}
				CountySurrogate lastSurrogate = county.surrogate(surrogateCodes[noOfSurrogates - 1]);
				delegate.printValueWithLineSeparator(value(lastSurrogate));
			}
		}
		delegate.close();
		System.out.println("Finished writting NOT1 file.");
	}

	private boolean atLeastOneNot1InARow(County county, int[] surrogateCodes) {
		for (int i = 0; i < surrogateCodes.length; i++) {
			CountySurrogate surrogate = county.surrogate(surrogateCodes[i]);
			if (surrogate != null && !delegate.isTotalOne(surrogate.getSum()) ) {
				return true;
			}
		}
		return false;
	}

	private String value(CountySurrogate surrogate) {
		if (surrogate == null) {
			return "";
		}

		StringBuffer sb = new StringBuffer();
		double sum = surrogate.getSum();
		boolean sumIsNotOne = !delegate.isTotalOne(sum);
		if (sumIsNotOne) {
			sb.append(delegate.format(sum));
		}

		return sb.toString();
	}

}
