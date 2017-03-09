package gov.epa.surrogate.gapfill;

import gov.epa.surrogate.Comment;
import gov.epa.surrogate.FileVerifier;
import gov.epa.surrogate.merge.InputFileReader;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.util.ArrayList;
import java.util.List;

public class GapfillInputFileReader {

	private BufferedReader reader;

	private String outputFileName;

	private String crossReferenceFileName;

	private List<GapfillCommand> commands;
	
	private List<String> inputFileLines;

	private InputFileReader delegate;

	public GapfillInputFileReader(String fileName) throws Exception {
		File file = new File(fileName);
		new FileVerifier().shouldExist(file);

		reader = new BufferedReader(new FileReader(file));
		commands = new ArrayList<GapfillCommand>();
		inputFileLines = new ArrayList<String>();
		delegate = new InputFileReader();
	}

	public void read() throws Exception {
		String line = null;
		Comment comment = new Comment();
		while ((line = reader.readLine()) != null) {
			inputFileLines.add(line);
			line = line.trim();
			if (!comment.isComment(line)) {
				process(delegate.tag(line), line);
			}
		}
		delegate.verify(outputFileName,crossReferenceFileName,commands.size(),"gapfill");
	}

	// TODO: tags case sensitive?
	private void process(String tag, String line) throws Exception {
		if (tag == null) {
			return;
		}
		if ("OUTFILE".equals(tag)) {
			this.outputFileName = delegate.outputFileName(tag, line);
		}
		if ("XREFFILE".equals(tag)) {
			this.crossReferenceFileName = delegate.crossReferenceFile(tag, line);
		}
		if ("OUTSRG".equals(tag)) {
			commandLine(line);
		}
	}

	private void commandLine(String line) throws Exception {
		// OUTSRG=Gapfilled Airports; GAPFILL=data\srg_nash_ref.txt|Airports;data\srg_nash_ref.txt|Population
		if (line.contains("GAPFILL")) {
			GapfillCommand command = new GapfillCommand(line);
			commands.add(command);
		}else{
			System.out.println("Warning: The line '"+line+"' does not contain 'GAPFILL' tag");
		}
	}

	public String getOutputFileName() {
		return outputFileName;
	}

	public String getCrossReferenceFileName() {
		return crossReferenceFileName;
	}

	public GapfillCommand[] getCommands() {
		return this.commands.toArray(new GapfillCommand[0]);
	}

	public List<String> getInputFileLines() {
		return inputFileLines;
	}

}
