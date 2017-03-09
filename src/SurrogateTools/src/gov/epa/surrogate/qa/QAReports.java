package gov.epa.surrogate.qa;

import gov.epa.surrogate.SrgDescriptionFileReader;
import gov.epa.surrogate.SurrogateDescription;
import gov.epa.surrogate.Surrogates;

import java.io.IOException;

public class QAReports {

	private SrgDescriptionFileReader srgDescriptionReader;

	private String srgDescFile;

	public QAReports(String fileName) throws Exception {
		this.srgDescFile= fileName;
		srgDescriptionReader = new SrgDescriptionFileReader(fileName);
	}
	
	public void execute(Threshold tt) throws Exception {
		readSrgDesc();
		String[] regions = srgDescriptionReader.getRegions();
		for (int i = 0; i < regions.length; i++) {
			execute(regions[i], tt);  // regions are USA, CANADA...
		}
	}
	
	public void execute(String regionName, Threshold tt) throws Exception {
		OutputFileNames fileNames = new OutputFileNames(srgDescFile,regionName);
		String summaryFileName = fileNames.summaryFileName();
		String gapFillFileName = fileNames.gapFillFileName();
		String noDataFileName = fileNames.noDataFileName();
		String not1FileName = fileNames.not1FileName();
		String threshFileName = fileNames.threshFileName();

		Counties counties = new Counties();
		readSrgFiles(regionName,counties, tt);
		
		Surrogates surrogates = srgDescriptionReader.getSurrogates(regionName);
		writeSrgSummary(summaryFileName, header(srgDescFile),counties, surrogates);
		writeGapFillSummary(gapFillFileName, header(srgDescFile), counties, surrogates);
		writeNoDataSummary(noDataFileName, header(srgDescFile),counties, surrogates);
		writeNot1Summary(not1FileName, header(srgDescFile),counties, surrogates);
		writeThreshSummary(threshFileName, header(srgDescFile),counties, surrogates, minTokens(), tt);

	}


	private void readSrgDesc() throws Exception {
		try {
			srgDescriptionReader.read();
		} catch (Exception e) {
			throw new Exception("Error reading the SRGDESC FILE '" + srgDescFile + "'\n" + e.getMessage());
		}
	}

	private void readSrgFiles(String regionName, Counties counties, Threshold tt) throws Exception {
		System.out.println("Reading surrogate files...");
		String fileName = null;
		SurrogateDescription[] surrogateDescriptions = srgDescriptionReader.getSurrogateDescriptions(regionName);
		if (surrogateDescriptions.length == 0) {
			System.out.println("No surrogate files were found in the SRGDESC file.");
		}
		try {
			for (int i = 0; i < surrogateDescriptions.length; i++) {
				fileName = surrogateDescriptions[i].getFileName();
				SurrogateFileReader fileReader = new SurrogateFileReader(surrogateDescriptions[i], "\t",
						srgDescriptionReader.getMinimumTokens(), counties);
				fileReader.read(tt);
			}
		} catch (Exception e) {
			throw new Exception("Error reading the surrogate file '" + fileName + "'\n" + e.getMessage());
		}
		System.out.println("Finished reading the surrogate information.");
	}

	
	// write srg summary information into a file
	private void writeSrgSummary(String summaryFileName, String header, Counties counties, Surrogates surrogates) throws Exception {
		try {
			SurrogateSummaryReport writer = new SurrogateSummaryReport(summaryFileName, counties, header, surrogates);
			writer.write();
		} catch (IOException e) {
			throw new Exception("Error writing the OUTPUT SUMMARY FILE: " + e.toString());
		}
	}

	private void writeGapFillSummary(String gapFillFileName, String header, Counties counties, Surrogates surrogates) throws Exception {
		try {
			SurrogateGapFillReport writer = new SurrogateGapFillReport(gapFillFileName, counties, header, surrogates);
			writer.write();
		} catch (IOException e) {
			throw new Exception("Error writing the GAPFILL SUMMARY FILE: " + e.toString());
		}
	}

	private void writeNoDataSummary(String noDataFileName, String header, Counties counties, Surrogates surrogates) throws Exception {
		try {
			SurrogateNoDataReport noData = new SurrogateNoDataReport(noDataFileName, counties, header, surrogates);
			noData.write();
		} catch (Exception e) {
			throw new Exception("Error writing the NODATA SUMMARY FILE --" + e.toString());
		}
	}

	private void writeNot1Summary(String not1FileName, String header, Counties counties, Surrogates surrogates) throws Exception {
		try {
			SurrogateNot1Report not1 = new SurrogateNot1Report(not1FileName, counties, header, surrogates);
			not1.write();
		} catch (Exception e) {
			throw new Exception("Error writing the NOT1 SUMMARY FILE --" + e.toString());
		}
	}
	
	private void writeThreshSummary(String threshFileName, String header, Counties counties, 
			Surrogates surrogates, int minTokens, Threshold tt) throws Exception {
		try {
			SurrogateThreshReport thresh = new SurrogateThreshReport(threshFileName, counties, header, surrogates, minTokens, tt);
			thresh.write();
		} catch (Exception e) {
			throw new Exception("Error writing the NOT1 SUMMARY FILE --" + e.toString());
		}
	}
	
	

	private String header(String srgDescFileName) {
		String header = srgDescriptionReader.getHeader() + System.getProperty("line.separator") + srgDescFileName;
		return header;
	}
	
	private int minTokens() {
		int tokes = srgDescriptionReader.getMinimumTokens();
		return tokes;
	}

}
