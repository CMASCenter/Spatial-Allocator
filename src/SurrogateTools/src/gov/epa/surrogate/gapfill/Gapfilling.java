package gov.epa.surrogate.gapfill;

import java.util.ArrayList;
import java.util.List;

import gov.epa.surrogate.Surrogates;

public class Gapfilling {

	private GapfillInputFileReader inputFileReader;

	private List<Counties> countiesColl;

	private ArrayList<String> srgInfoHeaderColl;

	private String gridHeader;

	public Gapfilling(String inputFile) throws Exception {
		inputFileReader = new GapfillInputFileReader(inputFile);
		countiesColl = new ArrayList<Counties>();
		srgInfoHeaderColl = new ArrayList<String>();
	}

	public void doGapfill() throws Exception {
		System.out.println("Reading gapfill input file...");
		inputFileReader.read();
		GapfillCommand[] commands = inputFileReader.getCommands();
		verifyGrids(commands);
		CrossReferenceFileReader xrefFileReader = new CrossReferenceFileReader(inputFileReader
				.getCrossReferenceFileName());
		xrefFileReader.read();
		Surrogates surrogates = xrefFileReader.getSurrogates();
		System.out.println("Finished reading gapfill input file and started gapfilling...");
		for (int i = 0; i < commands.length; i++) {
			GapfillCommandReader reader = new GapfillCommandReader(surrogates, commands[i]);
			reader.read();
			countiesColl.add(reader.getCounties());
			addInfoHeader(reader.getSrgInfoHeader());
		}
		GapfillOutput output = new GapfillOutput(inputFileReader.getOutputFileName());
		output.write(gridHeader, inputFileReader.getInputFileLines(), srgInfoHeaderColl, countiesColl);
		System.out.println("Finished gapfilling");
	}

	// checks for duplicate information
	private void addInfoHeader(List<String> srgInfoHeader) {
		for (int i = 0; i < srgInfoHeader.size(); i++) {
			String header = srgInfoHeader.get(i);
			if (!srgInfoHeaderColl.contains(header)) {
				srgInfoHeaderColl.add(header);
			}
		}
	}

	private void verifyGrids(GapfillCommand[] commands) throws Exception {
		GridChecker gridChecker = new GridChecker(commands);
		gridChecker.verify();
		this.gridHeader = gridChecker.gridHeader();
	}
}
