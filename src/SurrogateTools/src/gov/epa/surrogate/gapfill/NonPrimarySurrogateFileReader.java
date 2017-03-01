package gov.epa.surrogate.gapfill;

import gov.epa.surrogate.SurrogateFileInfo;
import gov.epa.surrogate.Surrogates;

import java.io.FileNotFoundException;

public class NonPrimarySurrogateFileReader {

	private SurrogateFileReader surrogateFileReader;

	public NonPrimarySurrogateFileReader(Surrogates surrogates, SurrogateFileInfo gapfillInfo, Counties counties, int outputSurrogateID)
			throws FileNotFoundException {
		this.surrogateFileReader = new SurrogateFileReader(surrogates, gapfillInfo, counties, outputSurrogateID);
	}

	public void read() throws Exception {
		surrogateFileReader.read(new NonPrimaryReplace());
	}
}
