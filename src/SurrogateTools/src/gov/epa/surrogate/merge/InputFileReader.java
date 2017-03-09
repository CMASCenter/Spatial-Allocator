package gov.epa.surrogate.merge;

import gov.epa.surrogate.FileVerifier;

import java.io.File;

public class InputFileReader {

	public String tag(String line) {
		int index = line.indexOf("=");
		if (index == -1) {
			return null;
		}
		return line.substring(0, index).trim();
	}

	public void verify(String outputFileName, String crossReferenceFileName, int commandSize, String function)
			throws Exception {
		if (outputFileName == null) {
			throw new Exception("The output surrogate file name is not specified");
		}

		if (crossReferenceFileName == null) {
			throw new Exception("The cross reference file name is not specified");
		}

		if (commandSize == 0) {
			throw new Exception("Atleast one " + function + " command should be specified");
		}

	}

	public String outputFileName(String tag, String line) throws Exception {
		String fileName = fileName(tag, line);
		if (new File(fileName).exists()) {
			System.out.println("Delete existing output surrogate file: " + fileName);
			new File(fileName).delete();
		}
		return fileName;
	}
	
	public String crossReferenceFile(String tag, String line) throws Exception {
		String fileName = fileName(tag, line);
		new FileVerifier().shouldExist(new File(fileName));
		return fileName;
	}
	
	private String fileName(String tag, String line) throws Exception {
		String[] tokens = line.split(tag);
		String msg = "The line '" + line + "' is not in the correct format";
		if (tokens.length != 2) {
			throw new Exception(msg);
		}
		String token = tokens[1].trim();
		if (!token.startsWith("=")) {
			throw new Exception("Expecting '=' after tag '" + tag + "'\n" + msg);
		}
		return token.substring(1);
	}

}
