package gov.epa.surrogate.normalize;

import gov.epa.surrogate.Comment;
import gov.epa.surrogate.FileVerifier;
import gov.epa.surrogate.Precision;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;

public class NormalizeSurrogate {

	private BufferedReader reader;

	private String delimiter;

	private int minimumTokens;

	private int previousCountyId;

	private PrintWriter writer;

	private int[] excludeCounties;

	private String normalizedFileName;

	private Precision precision;

	public NormalizeSurrogate(String fileName, int minimumTokens, Precision precision, int[] excludeCounties) throws Exception {
		File file = new File(fileName);
		new FileVerifier().shouldExist(file);
		reader = new BufferedReader(new FileReader(file));
		this.minimumTokens = minimumTokens;
		this.precision = precision;
		this.excludeCounties = excludeCounties;
		this.delimiter = "\t";
		this.previousCountyId = -1;

		NormalizationCheck normalizationCheck = new NormalizationCheck(fileName, minimumTokens, precision, delimiter);
		normalizationCheck.required();
		normalizedFileName = outputFileName(fileName);
		writer = new PrintWriter(new FileWriter(new File(normalizedFileName)), true);
	}

	private String outputFileName(String fileName) throws Exception {
		File file = new File(fileName);
		String dirName = file.getParent();
		if (dirName == null)
		{
			dirName = ".";
		}
		String outputFileName = dirName + File.separator + 
		        file.getName().substring(0,file.getName().lastIndexOf('.')) + "_NORM.txt";
		if (new File(outputFileName).exists()) {
			System.out.println("Delete existing normalized output surrogate file: " + outputFileName);
			new File(outputFileName).delete();		}
		return outputFileName;
	}

	public void normalize() throws IOException {
		String line = null;
		NormalizeLineParser parser = new NormalizeLineParser(delimiter, minimumTokens);
		Comment comment = new Comment();
		County county = null;
		boolean isHeader = true;
		while ((line = reader.readLine()) != null) {
			if (!comment.isComment(line)) {
				isHeader = false;
				NormalizeSurrogateRow row = parser.parse(line);
				if (isNewCounty(row.getCountyID())) {
					printCounty(county);// printout the county information saved in county
					county = new County(row.getCountyID(),precision);
				}
				if ( row.getPolyInCty())
					county.addRow(row);
			} 
			else if (isHeader ){
				printComment(line);
			}
		}
		printCounty(county);
		reader.close();
		writer.close();
	}

	private void printComment(String line) {
		writer.println(line);
	}

	private void printCounty(County county) {
		if (county == null)
			return;
		if (!excludeNormalize(county.getCountyID(), excludeCounties)) {
			county.normalize();
		}
		NormalizeSurrogateRow[] rows = county.surrogateRows();
		for (int i = 0; i < rows.length; i++) {
			writer.println(rows[i].getLine());
		}
	}

	private boolean excludeNormalize(int countyID, int[] excludeCounties) {
		for (int i = 0; i < excludeCounties.length; i++) {
			if (countyID == excludeCounties[i]) {
				return true;
			}
		}
		return false;
	}

	private boolean isNewCounty(int countyID) {
		boolean newCounty = true;

		if (previousCountyId == -1 || previousCountyId != countyID) {
			previousCountyId = countyID;
		} else if (previousCountyId == countyID) {
			newCounty = false;
		}
		return newCounty;
	}

	public String getNormalizedFileName() {
		return normalizedFileName;
	}

}
