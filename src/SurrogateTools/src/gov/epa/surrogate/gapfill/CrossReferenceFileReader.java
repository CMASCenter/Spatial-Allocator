package gov.epa.surrogate.gapfill;

import gov.epa.surrogate.SrgCodeLineParser;
import gov.epa.surrogate.Surrogates;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;

public class CrossReferenceFileReader {

	private BufferedReader reader;

	private Surrogates surrogates;

	public CrossReferenceFileReader(String fileName) throws FileNotFoundException {
		reader = new BufferedReader(new FileReader(new File(fileName)));
		surrogates = new Surrogates("Unspecified Region");
	}

	public void read() throws Exception {
		String line = null;
		SrgCodeLineParser parser = new SrgCodeLineParser(surrogates); 
		while ((line = reader.readLine()) != null) {
			line = line.trim();
			parser.parse(line);
		}
	}

	public Surrogates getSurrogates() {
		return surrogates;
	}
}
