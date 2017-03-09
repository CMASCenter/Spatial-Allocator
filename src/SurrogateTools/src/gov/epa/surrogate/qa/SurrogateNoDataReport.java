package gov.epa.surrogate.qa;

import gov.epa.surrogate.Surrogates;

import java.io.File;
import java.io.FileWriter;
import java.io.PrintWriter;

public class SurrogateNoDataReport {

	private Counties counties;

	private String delimiter;

	private String fileName;

	private SrgSummary srgSummary;

	public SurrogateNoDataReport(String fileName, Counties counties, String header, Surrogates surrogates)
			throws Exception {
		this.fileName = fileName;
		this.delimiter = ",";
		this.counties = counties;
		PrintWriter writer = new PrintWriter(new FileWriter(new File(fileName)));
		this.srgSummary = new SrgSummary(writer, delimiter, counties, header, surrogates, null);
	}

	public void write() throws Exception {
		System.out.println("The output surrogate NODATA file: " + fileName);

		int[] countyCodes = counties.allCountyCodes();
		int[] surrogateCodes = counties.allSurrgateCodes();
		int noOfSurrogates = surrogateCodes.length;
		srgSummary.printHeader();
		srgSummary.printColumnHeader(surrogateCodes);
		for (int i = 0; i < countyCodes.length; i++) {
			County county = counties.getCounty(countyCodes[i]);
			if (atLeastOneNoDataInARow(county, surrogateCodes)) {
				srgSummary.printValueWithDelimiter(county.getCountyCode());
				for (int j = 0; j < noOfSurrogates - 1; j++) {
					CountySurrogate surrogate = county.surrogate(surrogateCodes[j]);
					srgSummary.printValueWithDelimiter(value(surrogate));
				}
				CountySurrogate lastSurrogate = county.surrogate(surrogateCodes[noOfSurrogates - 1]);
				srgSummary.printValueWithLineSeparator(value(lastSurrogate));
			}
		}
		srgSummary.close();
		System.out.println("Finished writting NODATA file.");
	}

	private boolean atLeastOneNoDataInARow(County county, int[] surrogateCodes) {
		for (int i = 0; i < surrogateCodes.length; i++) {
			if (county.surrogate(surrogateCodes[i]) == null) {
				return true;
			}
		}
		return false;
	}

	private String value(CountySurrogate surrogate) {
		if (surrogate == null) {
			return "NODATA";
		}
		return "";
	}

}
