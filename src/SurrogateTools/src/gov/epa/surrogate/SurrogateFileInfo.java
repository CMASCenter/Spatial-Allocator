package gov.epa.surrogate;

import java.io.File;


public class SurrogateFileInfo {

	private String surrogateFileName;

	private String surrogateName;

	public SurrogateFileInfo(String infoToken, String line) throws Exception {
		String[] tokens = split(infoToken, line);
		this.surrogateFileName = tokens[0].trim();
		new FileVerifier().shouldExist(new File(surrogateFileName));
		this.surrogateName = tokens[1].trim();
	}

	private String[] split(String infoToken, String line) throws Exception {
		String[] tokens = infoToken.split("\\|");
		if (tokens.length != 2) {
			throw new Exception("The line '" + line + "' is not in the correct format");
		}
		return tokens;
	}

	public String getSurrogateFileName() {
		return surrogateFileName;
	}

	public String getSurrogateName() {
		return surrogateName;
	}

}
