package gov.epa.surrogate.gapfill;

import java.io.FileNotFoundException;
import java.util.List;

import gov.epa.surrogate.SurrogateFileInfo;
import gov.epa.surrogate.Surrogates;

public class PrimarySurrogateFileReader {

	private SurrogateFileReader surrogateFileReader;

	public PrimarySurrogateFileReader(Surrogates surrogates, SurrogateFileInfo info, Counties counties, int outputSurrogateID) throws FileNotFoundException {
		this.surrogateFileReader= new SurrogateFileReader(surrogates, info, counties, outputSurrogateID);
	}

	public List<String> getComments() {
		return surrogateFileReader.getComments();
	}

	public void read() throws Exception {
		surrogateFileReader.read(new PrimaryReplace());
	}
}
