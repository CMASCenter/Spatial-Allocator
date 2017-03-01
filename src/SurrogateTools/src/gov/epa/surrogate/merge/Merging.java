package gov.epa.surrogate.merge;

import gov.epa.surrogate.Surrogates;
import gov.epa.surrogate.gapfill.CrossReferenceFileReader;
import gov.epa.surrogate.gapfill.GridChecker;

import java.util.ArrayList;
import java.util.List;

public class Merging {

	private MergeInputFileReader inputFileReader;

	private ArrayList<String> srgInfoHeaderColl;

	private String gridHeader;

	private List<Counties> countiesColl;

	public Merging(String inputFile) throws Exception {
		inputFileReader = new MergeInputFileReader(inputFile);
		srgInfoHeaderColl = new ArrayList<String>();
		countiesColl = new ArrayList<Counties>();
	}

	public void doMerging() throws Exception {
		System.out.println("Reading merge input file...");
		inputFileReader.read();
		MergeCommand[] commands = inputFileReader.getCommands();
		verifyGrids(commands);
		CrossReferenceFileReader xrefFileReader = new CrossReferenceFileReader(inputFileReader
				.getCrossReferenceFileName());
		xrefFileReader.read();
		Surrogates surrogates = xrefFileReader.getSurrogates();
		System.out.println("Finished reading merge input file and started merging..." );
		for (int i = 0; i < commands.length; i++) {
			MergeCommandReader reader = new MergeCommandReader(surrogates, commands[i]);
			reader.read();		
			Counties mergedCounties = merge(reader.getCountiesColl(), commands[i].getEquation(), surrogates
					.getSurrogateID(commands[i].getOutputSurrogate()));
			countiesColl.add(mergedCounties);
			addInfoHeader(reader.getSrgInfoHeader());
		}
				
		MergeOutput output = new MergeOutput(inputFileReader.getOutputFileName());
		output.write(gridHeader, inputFileReader.getInputFileLines(), srgInfoHeaderColl, countiesColl);
		System.out.println("Finished merging");
	}

	private Counties merge(List<Counties> countiesColl, Equation equation, int outputSurrogateID) throws Exception {
		return new MergeCounties(countiesColl, equation, outputSurrogateID).doMerge();
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

	private void verifyGrids(MergeCommand[] commands) throws Exception {
		GridChecker gridChecker = new GridChecker(commands);
		gridChecker.verify();
		this.gridHeader = gridChecker.gridHeader();
	}
}
