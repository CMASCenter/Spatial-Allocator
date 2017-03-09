package gov.epa.surrogate.normalize;

import gov.epa.surrogate.Comment;
import gov.epa.surrogate.Precision;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;

public class NormalizationCheck {

	private BufferedReader reader;

	private int minimumTokens;

	private String delimiter;

	private int previousCountyId;

	private String fileName;

	private Precision precision;

	public NormalizationCheck(String fileName, int minimumTokens, Precision precision, String delimiter) throws FileNotFoundException {
		this.fileName = fileName;
		reader = new BufferedReader(new FileReader(new File(fileName)));
		this.minimumTokens = minimumTokens;
		this.precision = precision;
		this.delimiter = delimiter;
		this.previousCountyId = -1;
	}

	public void required() throws NormalizeException, IOException {
		String line = null;
		NormalizeLineParser parser = new NormalizeLineParser(delimiter, minimumTokens);
		County county = null;
		Comment comment = new Comment();
		while ((line = reader.readLine()) != null) {
			if (comment.isComment(line)) {
				continue;
			}
			NormalizeSurrogateRow row = parser.parse(line);
			if (isNewCounty(row.getCountyID())) {
				if (!checkForSumOne(county)) {
					return;
				}
				county = new County(row.getCountyID(),precision);
			}
			county.addRow(row);
		}
		reader.close();
		throw new NormalizeException("Normalization is not required for the surrogate file '" + fileName + "'");
	}

	private boolean checkForSumOne(County county) {
		if (county == null) {
			return true;
		}
		return county.isTotalOne();
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

}
