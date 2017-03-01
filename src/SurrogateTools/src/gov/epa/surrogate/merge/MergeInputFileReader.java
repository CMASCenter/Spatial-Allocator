package gov.epa.surrogate.merge;

import gov.epa.surrogate.Comment;
import gov.epa.surrogate.FileVerifier;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.util.ArrayList;
import java.util.List;

public class MergeInputFileReader {

	private BufferedReader reader;

	private String outputFileName;

	private String crossReferenceFileName;

	private List<MergeCommand> commands;

	private InputFileReader delegate;

	private List<String> inputFileLines;

	public MergeInputFileReader(String fileName) throws Exception {
		File file = new File(fileName);
		new FileVerifier().shouldExist(file);

		reader = new BufferedReader(new FileReader(new File(fileName)));
		commands = new ArrayList<MergeCommand>();
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
		delegate.verify(outputFileName, crossReferenceFileName, commands.size(), "merge");

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
		// OUTSRG=Half Pop Half Housing; 0.5*({data/surrogates.txt|Population})+0.5*({data/surrogates.txt|Housing})
		MergeCommand command = new MergeCommand(line);
		commands.add(command);
	}

	public String getOutputFileName() {
		return outputFileName;
	}

	public String getCrossReferenceFileName() {
		return crossReferenceFileName;
	}

	public MergeCommand[] getCommands() {
		return this.commands.toArray(new MergeCommand[0]);
	}

	public List<String> getInputFileLines() {
		return inputFileLines;
	}

}
