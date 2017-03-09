package gov.epa.surrogate.gapfill;

import gov.epa.surrogate.Command;
import gov.epa.surrogate.SurrogateFileInfo;

import java.util.ArrayList;
import java.util.List;

public class GapfillCommand  implements Command {

	private String outputSurrogate;

	private List<SurrogateFileInfo> gapfillInfos;

	// OUTSRG=Gapfilled Airports; GAPFILL=data\srg_nash_ref.txt|Airports;data\srg_nash_ref.txt|Population
	public GapfillCommand(String line) throws Exception {
		validateCommandLine(line);
		this.outputSurrogate = outputSurrogate(line);
		this.gapfillInfos = new ArrayList<SurrogateFileInfo>();
		gapfillInfos(line);
	}

	private void validateCommandLine(String line) throws Exception {
		if(!line.startsWith("OUTSRG")){
			throw new Exception("'OUTSRG' tag not found.\nThe line '" + line + "' is not in the correct format");
		}
		if(!line.contains("GAPFILL")){
			throw new Exception("'GAPFILL' tag not found.\nThe line '" + line + "' is not in the correct format");
		}
	}

	private void gapfillInfos(String line) throws Exception {
		String[] tokens = line.split("GAPFILL");
		String token = tokens[1].trim();
		if (!token.startsWith("=")) {
			throw new Exception("Missing '=' after GAPFILL tag.\nThe line '" + line + "' is not in the correct format");
		}
		token = token.substring(1).trim();// remove '=' and trim to remove any spaces
		String[] infoTokens = token.split(";");
		for (int i = 0; i < infoTokens.length; i++) {
			SurrogateFileInfo info = new SurrogateFileInfo(infoTokens[i], line);
			gapfillInfos.add(info);
		}
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
		return gapfillInfos.toArray(new SurrogateFileInfo[0]);
	}

}
