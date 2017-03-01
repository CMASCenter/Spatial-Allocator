package gov.epa.surrogate.qa;

import java.io.File;

public class OutputFileNames {

	private String srgDescriptionFileName;
	
	private String regionName;

	public OutputFileNames(String srgDescriptionFile, String regionName) {
		this.srgDescriptionFileName = srgDescriptionFile;
		this.regionName = regionName;
	}

	public String summaryFileName() throws Exception {
		return fileName(srgDescriptionFileName, "_" + regionName, "_summary.csv");
	}

	public String gapFillFileName() throws Exception {
		return fileName(srgDescriptionFileName, "_" + regionName, "_gapfill.csv");
	}

	public String not1FileName() throws Exception {
		return fileName(srgDescriptionFileName, "_" + regionName, "_not1.csv");
	}
	
	public String threshFileName() throws Exception {
		return fileName(srgDescriptionFileName, "_" + regionName, "_threshold.csv");
	}

	public String noDataFileName() throws Exception {
		return fileName(srgDescriptionFileName, "_" + regionName, "_nodata.csv");
	}

	private String fileName(String srgDescFile, String regionName, String suffix) throws Exception {
		String dirName = new File(srgDescFile).getParent();
		if (dirName == null)
		{
			dirName = ".";
		}
		String outputFileName = dirName + File.separator + prefix(srgDescFile) + regionName
				+ suffix;
		File outputfile = new File(outputFileName);
		if (outputfile.exists()) {
			throw new Exception("The output file '" + outputfile.getAbsolutePath() + " already exists");
		}
		return outputFileName;
	}

	private String prefix(String srgDescFile) {
		File file = new File(srgDescFile);
		String name = file.getName();
		int index = name.lastIndexOf(".");
		if (index == -1) {
			return name;
		}
		return name.substring(0, index);
	}

}
