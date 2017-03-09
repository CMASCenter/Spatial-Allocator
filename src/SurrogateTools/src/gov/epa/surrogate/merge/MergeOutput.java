package gov.epa.surrogate.merge;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.List;

public class MergeOutput {

	private PrintWriter writer;

	public MergeOutput(String outputFileName) throws IOException {
		writer = new PrintWriter(new FileWriter(new File(outputFileName)),true);
	}

	public void write(String gridHeader, List<String> inputFileLines, List<String> srgInfoHeaders, List<Counties> counties) throws Exception {
		writer.println(gridHeader);
		printInfoGridInfo(srgInfoHeaders);
		printInputFile(inputFileLines);
		printCountiesCollection(counties);
		writer.close();
	}

	private void printCountiesCollection(List<Counties> countiesCollection) throws Exception {
		for (int i = 0; i < countiesCollection.size(); i++) {
			Counties counties = countiesCollection.get(i);
			printCounties(counties);
		}
	}

	private void printCounties(Counties counties) throws Exception {
		List<Integer> allCountiesCodes = counties.allCountyCodes();
		for (int i = 0; i < allCountiesCodes.size(); i++) {
			County county = counties.getCounty(allCountiesCodes.get(i).intValue());
			List<SurrogateRow> rows = county.getSurrogates();
			for (int j = 0; j < rows.size(); j++) {
				writer.println(rows.get(j).getLine());
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
