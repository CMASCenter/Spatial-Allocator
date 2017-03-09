package gov.epa.surrogate.gapfill;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.List;

public class GapfillOutput {

	private PrintWriter writer;

	public GapfillOutput(String outputFileName) throws IOException {
		writer = new PrintWriter(new FileWriter(new File(outputFileName)),true);
	}

	public void write(String gridHeader, List<String> inputFileLines, List<String> srgInfoHeaders, List<Counties> counties) {
		writer.println(gridHeader);
		printInfoGridInfo(srgInfoHeaders);
		printInputFile(inputFileLines);
		printCountiesCollection(counties);
		writer.close();
	}

	private void printCountiesCollection(List<Counties> countiesCollection) {
		for (int i = 0; i < countiesCollection.size(); i++) {
			Counties counties = countiesCollection.get(i);
			printCounties(counties);
		}
	}

	private void printCounties(Counties counties) {
		County[] allCounties = counties.getCounties();
		for (int i = 0; i < allCounties.length; i++) {
			County county = allCounties[i];
			String[] rows = county.getRows();
			for (int j = 0; j < rows.length; j++) {
				writer.println(rows[j]);
			}
		}
	}

	private void printInfoGridInfo(List<String> srgInfoHeaders) {
		for (int i = 0; i < srgInfoHeaders.size(); i++) {
			writer.println(srgInfoHeaders.get(i));
		}
	}
	
	private void printInputFile(List<String> inputFileLines) {
		for (int i = 0; i < inputFileLines.size(); i++) {
			writer.println("#"+inputFileLines.get(i));
		}		
	}

}
