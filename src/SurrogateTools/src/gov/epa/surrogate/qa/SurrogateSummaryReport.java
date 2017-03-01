package gov.epa.surrogate.qa;

import gov.epa.surrogate.Surrogates;

import java.io.File;
import java.io.FileWriter;
import java.io.PrintWriter;

public class SurrogateSummaryReport {

	private Counties counties;

	private String delimiter;

	private String fileName;

	private SrgSummary srgSummary;

	public SurrogateSummaryReport(String fileName, Counties counties, String header, Surrogates surrogates)
			throws Exception {
		this.fileName = fileName;
		this.delimiter = ",";
		this.counties = counties;
		PrintWriter writer = new PrintWriter(new FileWriter(new File(fileName)));
		this.srgSummary = new SrgSummary(writer, delimiter, counties, header, surrogates, null);
	}

	public void write() throws Exception {
		System.out.println("The output surrogate summary file: " + fileName);

		int[] countyCodes = counties.allCountyCodes();
		int[] surrogateCodes = counties.allSurrgateCodes();
		int noOfSurrogates = surrogateCodes.length;
		srgSummary.printHeader();
		srgSummary.printColumnHeader(surrogateCodes);
		for (int i = 0; i < countyCodes.length; i++) {
			County county = counties.getCounty(countyCodes[i]);
			srgSummary.printValueWithDelimiter(county.getCountyCode());
			for (int j = 0; j < noOfSurrogates - 1; j++) {
				CountySurrogate surrogate = county.surrogate(surrogateCodes[j]);
				srgSummary.printValueWithDelimiter(value(surrogate));
			}
			CountySurrogate lastSurrogate = county.surrogate(surrogateCodes[noOfSurrogates - 1]);
			srgSummary.printValueWithLineSeparator(value(lastSurrogate));
		}
		srgSummary.close();
		System.out.println("Finished writting summary file.");
	}

	private String value(CountySurrogate surrogate) {
		if (surrogate == null) {
			return "NODATA";
		}

		StringBuffer sb = new StringBuffer();
		double sum = surrogate.getSum();
		boolean sumIsNotOne = !srgSummary.isTotalOne(sum);
		if (sumIsNotOne) {
			sb.append("NOT1:" + srgSummary.format(sum));
		}
		if (surrogate.isGapFillted()) {
			if (sumIsNotOne) {
				sb.append(";");
			}
			sb.append("GF:" + surrogate.gapFillCode());
		}

		return sb.toString();
	}

}
