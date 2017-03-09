package gov.epa.surrogate.merge;

import gov.epa.surrogate.Command;
import gov.epa.surrogate.SurrogateFileInfo;

import java.util.ArrayList;
import java.util.List;

public class MergeCommand implements Command {

	private String outputSurrogate;

	private List<SurrogateFileInfo> mergeInfos;

	private Equation equation;

	// OUTSRG=Half Pop Half Housing; 0.5*({data/surrogates.txt|Population})+0.5*({data/surrogates.txt|Housing})
	public MergeCommand(String line) throws Exception {
		validateCommandLine(line);
		this.outputSurrogate = outputSurrogate(line);
		this.mergeInfos = new ArrayList<SurrogateFileInfo>();
		mergeInfos(line);
	}

	private void validateCommandLine(String line) throws Exception {
		if (!line.startsWith("OUTSRG")) {
			throw new Exception("'OUTSRG' tag not found.\nThe line '" + line + "' is not in the correct format");
		}
	}

	private void mergeInfos(String line) throws Exception {
		String[] tokens = line.split(";");
		if (tokens.length != 2) {
			throw new Exception("The line '" + line + "' is not in the correct format");
		}

		String token = tokens[1].trim();
		EquationParser equationParser = new EquationParser(token, line);
		List<SurrogateFileInfo> infos = equationParser.parse();
		mergeInfos.addAll(infos);
		equation = equationParser.getEquation();
	}

	private String outputSurrogate(String line) throws Exception {
		int start = line.indexOf("=");
		int end = line.indexOf(";");
		try {
			return line.substring(start + 1, end).trim();
		} catch (IndexOutOfBoundsException e) {
			throw new Exception("The line '" + line + "' is not in the correct format");
		}
	}

	public String getOutputSurrogate() {
		return outputSurrogate;
	}

	public SurrogateFileInfo[] getSurrogateInfos() {
		return mergeInfos.toArray(new SurrogateFileInfo[0]);
	}

	public Equation getEquation() {
		return equation;
	}

}
