package gov.epa.surrogate.qa;

import gov.epa.surrogate.FileVerifier;
import gov.epa.surrogate.SurrogateDescription;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;

public class SurrogateFileReader {

	private BufferedReader fileReader;

	private String delimiter;

	private int minimumTokens;

	private boolean gapFilled;

	private Counties counties;

	private SurrogateDescription srgDescription;

	private String fileName;

	public SurrogateFileReader(SurrogateDescription description, String delimiter, int minimumTokens, Counties counties)
			throws Exception {
		fileName = description.getFileName();
		File file = verifyFile(fileName);
		this.srgDescription = description;
		this.fileReader = new BufferedReader(new FileReader(file));
		this.delimiter = delimiter;
		this.minimumTokens = minimumTokens;
		this.gapFilled = isGapFilled(fileName);
		this.counties = counties;
	}

	private File verifyFile(String fileName) throws Exception {
		File file = new File(fileName);
		new FileVerifier().shouldExist(file);
		return file;
	}

	private boolean isGapFilled(String name) {
		return !name.contains("NOFILL");
	}

	public void read(Threshold tt) throws IOException {
		System.out.println("Reading " + fileName);

		String line = null;
		LineParser parser = new LineParser(delimiter, minimumTokens);
		while ((line = fileReader.readLine()) != null) {
			if (!isComment(line)) {
				SurrogateRow row = parser.parse(line, gapFilled);
				if (verify(row)) {
					boolean isBig=tt.isTotalBig(row.getRatio());
					counties.add(row, isBig);
				}
			}
		}
		fileReader.close();
	}

	// rules
	private boolean verify(SurrogateRow row) {
		// if the surrogate id is different from surrogate id in the description file
		return row.getSurrogateCode() == srgDescription.getSurrogateCode();
	}

	private boolean isComment(String line) {
		line = line.trim();
		return line.length() == 0 || line.startsWith("#");
	}
}
