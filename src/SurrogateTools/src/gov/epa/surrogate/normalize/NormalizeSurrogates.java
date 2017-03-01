package gov.epa.surrogate.normalize;

import gov.epa.surrogate.FileVerifier;
import gov.epa.surrogate.Precision;
import gov.epa.surrogate.SrgDescriptionFileReader;
import gov.epa.surrogate.SurrogateDescription;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;

public class NormalizeSurrogates {

	private int[] excludeCounties;

	private PrintWriter normalizeSrgDescFile;

	private SrgDescriptionFileReader srgDescFileReader;

	private String delimiter;

	private Precision precision;

	public NormalizeSurrogates(String srgDescriptionFileName) throws Exception {
		new FileVerifier().shouldExist(new File(srgDescriptionFileName));
		this.precision = new Precision();
		this.excludeCounties = new int[0];
		setup(srgDescriptionFileName);
	}

	public NormalizeSurrogates(String srgDescriptionFileName, String secondArgument) throws Exception {
		new FileVerifier().shouldExist(new File(srgDescriptionFileName));
		verify(srgDescriptionFileName, secondArgument);
		this.precision = new Precision();	
		this.excludeCounties = new ExcludeCountiesFileReader(secondArgument).read();				
		setup(srgDescriptionFileName);
	}

	public NormalizeSurrogates(String srgDescriptionFileName, String excludeCountiesFileName, String precision)
			throws Exception {
		verify(srgDescriptionFileName, excludeCountiesFileName);
		this.precision = new Precision(precision(precision));
		this.excludeCounties = new ExcludeCountiesFileReader(excludeCountiesFileName).read();
		setup(srgDescriptionFileName);
	}

	private void setup(String srgDescriptionFileName) throws Exception, IOException {
		this.srgDescFileReader = new SrgDescriptionFileReader(srgDescriptionFileName);
		this.normalizeSrgDescFile = new PrintWriter(new FileWriter(new File(newSrgDescFile(srgDescriptionFileName))),
				true);
		this.delimiter = ",";
	}

	private double precision(String precision) throws Exception {
		try {
			return Double.parseDouble(precision.trim());
		} catch (NumberFormatException e) {
			throw new Exception(precision + " is not a double (numeric) value");
		}
	}

	private String newSrgDescFile(String fileName) throws Exception {
		File file = new File(fileName);
		String dirName = file.getParent();
		if (dirName == null)
			dirName = ".";
		String outputFileName = dirName + File.separator + 
		  file.getName().substring(0,file.getName().lastIndexOf('.')) + "_NORM.txt";
		if (new File(outputFileName).exists()) {
			System.out.println("Delete existing output surrogate file: " + outputFileName);
			new File(outputFileName).delete();
		}
		return outputFileName;
	}

	private void verify(String srgDescriptionFileName, String excludeCountiesFileName) throws Exception {
		FileVerifier verifier = new FileVerifier();
		verifier.shouldExist(new File(srgDescriptionFileName));
		verifier.shouldExist(new File(excludeCountiesFileName));
	}

	public void normalize() throws Exception {
		srgDescFileReader.read();
		int minimumTokens = srgDescFileReader.getMinimumTokens();
		SurrogateDescription[] surrogateDescriptions = srgDescFileReader.getSurrogateDescriptions();
		printComments(srgDescFileReader.getComments());
		for (int i = 0; i < surrogateDescriptions.length; i++) {
			System.out.println("Processing surrogate "+surrogateDescriptions[i]);
			try {
				NormalizeSurrogate ns = new NormalizeSurrogate(surrogateDescriptions[i].getFileName(), minimumTokens,
						precision, excludeCounties);
				ns.normalize();
				printSrgDescFile(surrogateDescriptions[i], ns.getNormalizedFileName());
			} catch (NormalizeException e) {
				printSrgDescFile(surrogateDescriptions[i], surrogateDescriptions[i].getFileName());
				System.out.println(e.getMessage());
			}
		}
		normalizeSrgDescFile.close();
	}

	private void printSrgDescFile(SurrogateDescription desc, String fileName) {
		normalizeSrgDescFile.print(desc.getRegion() + delimiter);
		normalizeSrgDescFile.print(desc.getSurrogateCode() + delimiter);
		normalizeSrgDescFile.print("\"" + desc.getSurrogateName() + "\"" + delimiter);
		normalizeSrgDescFile.println(fileName);
	}

	private void printComments(String[] comments) {
		for (int i = 0; i < comments.length; i++) {
			normalizeSrgDescFile.println(comments[i]);
		}
	}

}
