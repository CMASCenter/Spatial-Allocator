package gov.epa.surrogate;

import gov.epa.surrogate.SystemInfo;
import gov.epa.surrogate.gapfill.Gapfilling;
import gov.epa.surrogate.merge.Merging;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Date;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.List;
import java.util.Vector;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * This program is used to generate surrogate ratios by calling srgcreate.exe and srgmerger.exe or java
 * merging/gapfilling tools based on csv and txt files.
 * 
 * Usage: java SurrogateTool control_variables_grid.csv Requirement: srgcreate.exe, srgmerge.exe, SurrogateTools.jar and
 * java 2 Platform Standard Edition (J2SE) software Development Date: Auguat - November, 2005 May 2006: added egrid
 * computation Developed by: Limei Ran, Carolina Environmental Program at UNC
 */
public class SurrogateTool {

	long      startTime, endTime;     // time in milliseconds
	
	Hashtable controls; // main control variables

	Hashtable shapefiles; // shapefile information

	Hashtable surrogates; // surrogate generation information

	Hashtable generations; // surrogates to be generated

	Hashtable surrogateIDs; // surrogate code information

	Hashtable srgDesc = new Hashtable();; // surrogate description information

	Hashtable runLog = new Hashtable(); // run execution status for each surrogate

	String logFile = null; // run log file name with format: srgRun_"date"_"time".log

	public static final int runFinish = 0; // run finish without error

	public static final int runContinue = 1; // run continue without error

	public static final int runError = 2; // run continue with error

	public static final int runStop = 3; // run stop due to error

	int prunStatus = 0; // overall status of the program run, initial value = 0 = runFinish

	String USER; // User who is running the program

	String CS, OS, FS, LS; // system related information: computer system, operating type, file separator, line
							// separator

	String COMMENT, CMD, EQ; // batch file or csh file controls: comment sign, set environment variable command,
								// equal sign

	String TIME;

	String LIBPATH; // LD_LIBRARY_PATH for unix system

	String CONTROL_VARIABLE_FILE; // main control variable file for headers

	String SCRIPT_HEADER; // header for scripts .bat or .csh file

	String GRIDPOLY_HEADER; // GRID header for surrogates

	String[] COMMAND = new String[3]; // command to run external exe

	Vector mainEnv = new Vector(); // array to hold main environment variables

	public static final String NOFILL_TYPE = "_NOFILL"; // srg file type without gapfills

	public static final String FILL_TYPE = "_FILL"; // srg file type with gapfills

	public static final String tDir = "temp_files"; // temp file directory name

	public static final String rowKey = "ROWKEY"; // key for list containg all keys in the csv

	boolean run_create = false, run_merge = false, run_gapfill = false; // indicator to create, merge, and gapfill
																		// surrogates
    boolean hasSrgMerge = true;
	
	// method launch log file and to set system related variables
	public void getSystemInfo() {

		//get the current time in  milliseconds
		startTime = System.currentTimeMillis(); 
		
		USER = System.getProperty("user.name");
		CS = System.getProperty("os.name").toLowerCase();
		OS = CS.toLowerCase();
		FS = System.getProperty("file.separator");
		LS = System.getProperty("line.separator");
		System.out.println("Your Operating system is:  " + OS + LS);

		if (OS.indexOf("windows 9") > -1 || OS.indexOf("nt") > -1 || OS.indexOf("windows 20") > -1
				|| OS.indexOf("windows xp") > -1) {
			OS = "window";
			COMMENT = ":: ";
			CMD = "set";
			EQ = "=";
			TIME = "time /t";
			SCRIPT_HEADER = "@echo off";
			COMMAND[0] = "cmd.exe";
			COMMAND[1] = "/c";
			if (OS.indexOf("windows 9") > -1) {
				COMMAND[0] = "command.com";
			}
		} else {
			// assume Unix
			OS = "unix";
			COMMENT = "# ";
			CMD = "setenv";
			EQ = " ";
			TIME = "time";
			LIBPATH = System.getenv("LD_LIBRARY_PATH");
			// System.out.println("LD_LIBRARY_PATH: "+LIBPATH);
			SCRIPT_HEADER = "#!/bin/csh -f";
			COMMAND[0] = "/bin/csh";
			COMMAND[1] = "-c";
		}
	}

	public void createLogFile(String logFileName) {		
		
		// get running date and old date log file
		// SimpleDateFormat fmt = new SimpleDateFormat("yyyyMMdd_HHmmss");
		Date now = new Date();
		// String dt = fmt.format(now);
		// String logFileName = "srgRun_"+dt+".log";

		File cfile = new File(logFileName);
		File pfile = cfile.getParentFile();
		if (pfile == null) {
			// only specify name, so use the current directory for log file
			logFile = "." + FS + logFileName; // current directory
		} else {
			// check and make the directory of specified log file
			if (!pfile.exists()) {
				if (!pfile.mkdirs()) {
					System.out.println("Error: Creating the log file directory failed -- " + logFileName);
					System.exit(runStop);
				}
			}
			logFile = logFileName;
		}

		// move and set logFile
		int i = 1;
		String tempFile = logFile;
		while (checkFile(tempFile, "NONE", runError)) {
			tempFile = logFile + "." + Integer.toString(i);
			i++;
		}

		// rename existing log file to logFile.i
		if (!tempFile.equals(logFile)) {
			if (!renameFile(logFile, tempFile, runContinue)) {
				checkFile(logFile, "YES", runError); // delete the existing log file if renaming failed
			}
		}

		writeLogFile("\nRun Date: " + now.toString() + LS + LS, runContinue);
		System.out.println("\nRun Date: " + now.toString() + LS + LS);
	}

	// methods to track errors
	public void writeLogFile(String message, int runStatus) {

		// update run status
		if (runStatus != runFinish && runStatus != runContinue) {
			prunStatus = runStatus;
		}

		if (logFile == null) {
			System.out.println(message);
		} else {
			try {
				FileWriter fw = new FileWriter(logFile, true); // open logFile to append
				BufferedWriter logOut = new BufferedWriter(fw); // output file writer for log file
				logOut.write(message);

				if (runStatus == runFinish || runStatus == runStop) {
					String srgRunInfo = writeSrgRunLog();
					if (srgRunInfo != null) {
						logOut.write(srgRunInfo);
					}

					Date endDate = new Date();
					logOut.write(LS + "End Date: " + endDate.toString() + LS);
					System.out.println(LS + "End Date: " + endDate.toString() + LS);
					
					endTime = System.currentTimeMillis(); 
					double elapsed_time_minutes = (endTime - startTime)/(1000.0*60.0);
					logOut.write("Elapsed time in minutes: " + Double.toString(elapsed_time_minutes) + LS);
					System.out.println(LS + "Elapsed time in minutes: " + Double.toString(elapsed_time_minutes) + LS);
					
					if (runStatus == runStop) {
						logOut.write(LS + "ERROR -- The Program Run Stopped" + LS);
						System.out.println(LS + "ERROR -- The Program Run Stopped. See log file for details." + LS);
					}
					if (runStatus == runFinish && prunStatus == runFinish) {
						logOut.write(LS + "SUCCESS -- The Program Run Completed" + LS);
						System.out.println(LS + "SUCCESS -- The Program Run Completed. See log file for details." + LS);
					}
					if (runStatus == runFinish && prunStatus != runFinish) {
						logOut.write(LS + "FINISH -- The Program Run Finished with Some Errors" + LS);
						System.out.println(LS + "FINISH -- The Program Run Finished with Some Errors. See log file for details." + LS);
					}					
					
				}
				logOut.close();
			} catch (IOException e) {
				System.out.println("Error -- " + e.toString());
			}
		}

		if (runStatus == runFinish || runStatus == runStop) {
			System.exit(prunStatus); // program exit with prunStatus code
		}
	}

	//check a file is directory or not
	public boolean checkIsDirectory( String fileName) {
		
		String tfile = fileName;	
		File tempfile = new File(tfile);
		
        //check whether it exists and it is a directory
		if ( tempfile.isDirectory() ) {
			return true;			
		}	
		
		return false;
		
	}
	
	// method to check the existence of a file
	public boolean checkFile(String fileName, String delete, int runStatus) {

		String ow = delete.toUpperCase();
		String tfile = fileName;
		File tempfile = new File(tfile);

		// check the file, if it exists, delete it
		if (tempfile.exists() && ow.equals("YES")) {
			boolean success = tempfile.delete();
			if (!success) {
				writeLogFile("Error: Deleting " + tfile + " file" + LS, runStatus);
				return false;
			}
		}

		// the file has to be existing, otherwise error message
		if (!tempfile.exists() && ow.equals("NO")) {
			writeLogFile("Error: File - " + tfile + " - Does Not Exist" + LS, runStatus);
			return false;
		}

		// check the file existence, no message
		if (!tempfile.exists() && ow.equals("NONE")) {
			return false;
		}

		return true;
	}

	// method to check the existence of a directory
	public boolean checkDir(String dirName, int runStatus) {
		String tdir;

		tdir = dirName;
		File tempdir = new File(tdir);
		if (!tempdir.exists()) {
			boolean success = tempdir.mkdirs(); // create all directories
			if (!success) {
				writeLogFile("Error: Making the output directory -- " + dirName + LS, runStatus);
				return false;
			}
		}
		return true;
	}

	public void checkCSVError(Hashtable hTable, int runStatus) {
		ArrayList list = new ArrayList();
		int ERROR_INDEX = 0;

		if (hTable.containsKey("CSV_ERROR")) {
			list = (ArrayList) hTable.get("CSV_ERROR");
			String message = (String) list.get(ERROR_INDEX); // get error message
			writeLogFile(message + LS, runStatus);
		}
	}

	public void readControls(String fileName) {
		String[] items = { "VARIABLE", "VALUE" }; // first field--key field
		String[] gridVars = { "GENERATION CONTROL FILE", "SURROGATE SPECIFICATION FILE", "SHAPEFILE CATALOG",
				"SURROGATE CODE FILE", "SRGCREATE EXECUTABLE", "DEBUG_OUTPUT", "OUTPUT_FORMAT", "OUTPUT_FILE_TYPE",
				"OUTPUT_GRID_NAME", "GRIDDESC", "OUTPUT_FILE_ELLIPSOID", "OUTPUT DIRECTORY", "OUTPUT SURROGATE FILE",
				"OUTPUT SRGDESC FILE", "OVERWRITE OUTPUT FILES", "LOG FILE NAME", "COMPUTE SURROGATES FROM SHAPEFILES",
				"MERGE SURROGATES", "GAPFILL SURROGATES", "SHAPEFILE DIRECTORY", "DENOMINATOR_THRESHOLD" }; // last two
																											// are
																											// optional
		String[] polyVars = { "GENERATION CONTROL FILE", "SURROGATE SPECIFICATION FILE", "SHAPEFILE CATALOG",
				"SURROGATE CODE FILE", "SRGCREATE EXECUTABLE", "DEBUG_OUTPUT", "OUTPUT_FORMAT", "OUTPUT_FILE_TYPE",
				"OUTPUT_POLY_FILE", "OUTPUT_POLY_ATTR", "OUTPUT DIRECTORY", "OUTPUT SURROGATE FILE",
				"OUTPUT SRGDESC FILE", "OVERWRITE OUTPUT FILES", "LOG FILE NAME", "COMPUTE SURROGATES FROM SHAPEFILES",
				"MERGE SURROGATES", "GAPFILL SURROGATES", "SHAPEFILE DIRECTORY", "DENOMINATOR_THRESHOLD" }; // last two
																											// are
																											// optional
		String[] egridVars = { "GENERATION CONTROL FILE", "SURROGATE SPECIFICATION FILE", "SHAPEFILE CATALOG",
				"SURROGATE CODE FILE", "SRGCREATE EXECUTABLE", "DEBUG_OUTPUT", "OUTPUT_FORMAT", "OUTPUT_FILE_TYPE",
				"OUTPUT_GRID_NAME", "GRIDDESC", "OUTPUT_FILE_ELLIPSOID", "OUTPUT_POLY_FILE", "OUTPUT DIRECTORY",
				"OUTPUT SURROGATE FILE", "OUTPUT SRGDESC FILE", "OVERWRITE OUTPUT FILES", "LOG FILE NAME",
				"COMPUTE SURROGATES FROM SHAPEFILES", "MERGE SURROGATES", "GAPFILL SURROGATES", "SHAPEFILE DIRECTORY",
				"DENOMINATOR_THRESHOLD" }; // last two are optional

		int keyItems = 1; // 1 = first item is the key item
		ArrayList list = new ArrayList();
		int FIRST_ITEM = 0;
		boolean checkStatus;
		int i, j;

		if (!checkFile(fileName, "NO", runContinue)) {
			System.out.println("Error: Control File does not exist -- " + fileName);
			System.exit(runStop);
		}

		// get full path control variable file
		File cf = new File(fileName);
		try {
			CONTROL_VARIABLE_FILE = cf.getCanonicalPath(); // save for header
		} catch (Exception e) {
			e.printStackTrace();
			System.exit(runStop);
		}

		CSV file = new CSV(fileName);
		controls = file.readCSV(items, keyItems);
		checkCSVError(controls, runStop); // check errors in reading csv file
		
		// if the srgmerge executable is there with an empty value, behave as if it's not there
		// if srgmerge executable is not specified, Java merging and gapfilling should be used
		if (controls.get("SRGMERGE EXECUTABLE") == null)
     	{
			hasSrgMerge = false;
		}
		else if (!controls.get("SRGMERGE EXECUTABLE").toString().toLowerCase().contains("srgmerge"))
     	{
			hasSrgMerge = false;  //Force to use Java Gapfilling and Merging
		}
		if (hasSrgMerge)
		   System.out.println("Using SRGMERGE EXECUTABLE for gapfilling and merging");
		else
			System.out.println("Using Java version of gapfilling and merging");

		// first check and set log file path
		if (!controls.containsKey("LOG FILE NAME")) {
			writeLogFile("Error: Main control CSV file does not contain variable -- LOG FILE NAME" + LS, runStop);
		}
		list = (ArrayList) controls.get("LOG FILE NAME");
		logFile = (String) list.get(FIRST_ITEM);
		if (logFile.length() == 0 || logFile.matches("^\\s*$")) {
			logFile = null;
			writeLogFile("Error: Main control variable value for LOG FILE NAME is missing... " + LS, runStop);
		}
		createLogFile(logFile); // create log file name

		writeLogFile(LS + "\t\t" + "Main Control CSV File" + LS + LS, runContinue);
		// System.out.println("\t\tMain Control CSV File"+LS+LS);
		ArrayList rows = (ArrayList) controls.get(rowKey);
		for (j = 0; j < rows.size(); j++) {
			String key = (String) rows.get(j);
			if (!key.startsWith("SRGMERGE"))
			{
				list = (ArrayList) controls.get(key);
				String value = (String) list.get(0);
				// System.out.println("key: "+key+"\t"+value);
				writeLogFile(key + "\t" + value + LS, runContinue);
				if (value.length() == 0 || value.matches("^\\s*$")) {
					writeLogFile("Error: " + key + " does not have a value in " + CONTROL_VARIABLE_FILE + "." + LS, runStop);
				}
			}
		}
		writeLogFile(LS, runContinue);

		// check all needed variables exist
		String outputFType = getControls("OUTPUT_FILE_TYPE");
		if (outputFType.equals("RegularGrid")) {
			for (j = 0; j < gridVars.length - 2; j++) {
				if (!controls.containsKey(gridVars[j])) {
					writeLogFile("Error: " + gridVars[j] + " does not exist in " + CONTROL_VARIABLE_FILE + "." + LS,
							runStop);
				}
			}
		} else if (outputFType.equals("Polygon")) {
			for (j = 0; j < polyVars.length - 2; j++) {
				if (!controls.containsKey(polyVars[j])) {
					writeLogFile("Error: " + polyVars[j] + " does not exist in " + CONTROL_VARIABLE_FILE + "." + LS,
							runStop);
				}
			}
			/*if (!hasSrgMerge)
			   writeLogFile("Error: Java merging and gapfilling is not supported for Polygon files"+ LS,
					  runStop);
            */
		} else if (outputFType.equals("EGrid")) {
			for (j = 0; j < egridVars.length - 2; j++) {
				if (!controls.containsKey(egridVars[j])) {
					writeLogFile("Error: " + egridVars[j] + " does not exist in " + CONTROL_VARIABLE_FILE + "." + LS,
							runStop);
				}
			}
		}

		// check variables not needed
		for (j = 0; j < rows.size(); j++) {
			String key = (String) rows.get(j);
			checkStatus = false;
			if (outputFType.equals("RegularGrid")) {
				for (i = 0; i < gridVars.length; i++) {
					if (key.equals(gridVars[i])) {
						checkStatus = true;
					}
				}
			}
			if (outputFType.equals("Polygon")) {
				for (i = 0; i < polyVars.length; i++) {
					if (key.equals(polyVars[i])) {
						checkStatus = true;
					}
				}
			}
			if (outputFType.equals("EGrid")) {
				for (i = 0; i < egridVars.length; i++) {
					if (key.equals(egridVars[i])) {
						checkStatus = true;
					}
				}
			}

			if (!checkStatus && (! key.equals("SRGMERGE EXECUTABLE"))) {
				writeLogFile("Warning: " + key + " is not needed in " + CONTROL_VARIABLE_FILE + "." + LS, runContinue);
			}
		}

		checkStatus = checkFile(getControls("SRGCREATE EXECUTABLE"), "NO", runStop);
		if (hasSrgMerge) {
			checkStatus = checkFile(getControls("SRGMERGE EXECUTABLE"), "NO", runStop);
		}
		if (outputFType.equals("RegularGrid") || outputFType.equals("EGrid")) {
			checkStatus = checkFile(getControls("GRIDDESC"), "NO", runStop);
		}

		// check varaibles settings
		if (!getControls("DEBUG_OUTPUT").equals("Y") && !getControls("DEBUG_OUTPUT").equals("N")) {
			writeLogFile("Error: DEBUG_OUTPUT value has to be Y or N" + LS, runStop);
		}

		if (!getControls("OUTPUT_FORMAT").equals("SMOKE")) {
			writeLogFile("Error: OUTPUT_FORMAT value has to be SMOKE" + LS, runStop);
		}

		if (!outputFType.equals("RegularGrid") && !outputFType.equals("Polygon") && !outputFType.equals("EGrid")) {
			writeLogFile("Error: OUTPUT_FILE_TYPE value has to be RegularGrid, EGrid or Polygon" + LS, runStop);
		}

		//check two output files
		String outputSrgFile = getControls("OUTPUT SURROGATE FILE");
		if (checkIsDirectory(outputSrgFile) ){
			writeLogFile("Error: OUTPUT SURROGATE FILE - " + outputSrgFile + " is an existing directory." + LS, runStop);
		}
			
		outputSrgFile = getControls("OUTPUT SRGDESC FILE");
		if (checkIsDirectory(outputSrgFile)){
			writeLogFile("Error: OUTPUT SRGDESC FILE - " + outputSrgFile + " is an existing directory." + LS, runStop);
		}
		
		String ow = getControls("OVERWRITE OUTPUT FILES");
		String OW = ow.toUpperCase();
		if (!OW.equals("YES") && (!OW.equals("NO"))) {
			writeLogFile("Error: Invalid OVERWRITE OUTPUT FILES entry in the main control file" + LS, runStop);
		}

		ow = getControls("COMPUTE SURROGATES FROM SHAPEFILES");
		OW = ow.toUpperCase();
		if (!OW.equals("YES") && (!OW.equals("NO"))) {
			writeLogFile("Error: Invalid COMPUTE SURROGATES FROM SHAPEFILES entry in the main control file" + LS,
					runStop);
		}
		if (OW.equals("YES")) {
			run_create = true;
		}

		ow = getControls("MERGE SURROGATES");
		OW = ow.toUpperCase();
		if (!OW.equals("YES") && (!OW.equals("NO"))) {
			writeLogFile("Error: Invalid MERGE SURROGATES entry in the main control file" + LS, runStop);
		}
		if (OW.equals("YES")) {
			run_merge = true;
		}

		ow = getControls("GAPFILL SURROGATES");
		OW = ow.toUpperCase();
		if (!OW.equals("YES") && (!OW.equals("NO"))) {
			writeLogFile("Error: Invalid GAPFILL SURROGATES entry in the main control file" + LS, runStop);
		}
		if (OW.equals("YES")) {
			run_gapfill = true;
		}
	}

	// method to get control value for an item in control table
	public String getControls(String varName) {
		String value;
		ArrayList list = new ArrayList();
		int FIRST_ITEM = 0;

		if (!controls.containsKey(varName)) {
			writeLogFile("Error: Main control CSV file does not contain variable name -- " + varName + "" + LS, runStop);
		}
		list = (ArrayList) controls.get(varName);
		value = (String) list.get(FIRST_ITEM);
		if (value.length() == 0 || value.matches("^\\s*$")) {
			writeLogFile("Error: Main control variable value is missing -- here" + varName + LS, runStop);
		}
		return value;
	}

	public void readSurrogates() {
		String[] items = { "REGION", "SURROGATE CODE", "SURROGATE", "DATA SHAPEFILE", "DATA ATTRIBUTE",
				"WEIGHT SHAPEFILE", "WEIGHT ATTRIBUTE", "WEIGHT FUNCTION", "FILTER FUNCTION", "MERGE FUNCTION",
				"SECONDARY SURROGATE", "TERTIARY SURROGATE", "QUARTERNARY SURROGATE" }; // REGION+SURROGATE CODE = key
																						// field
		int keyItems = 2; // 2 = combined first two items is the key
		// ArrayList list = new ArrayList();
		// Iterator it;

		/** get Surrogate Specification File Name from control Hashtable* */
		String fileName = getControls("SURROGATE SPECIFICATION FILE");

		checkFile(fileName, "NO", runStop); // check CSV fileName exists

		CSV file = new CSV(fileName);
		surrogates = file.readCSV(items, keyItems);
		checkCSVError(surrogates, runStop);

		/**
		 * System.out.println(LS+"\t\t"+"Surrogate Specification CSV File"+LS+LS); ArrayList rows = (ArrayList)
		 * surrogates.get(rowKey); for (int j=0;j<rows.size();j++) { String key = (String) rows.get(j);
		 * System.out.println("key: "+key); list = (ArrayList) surrogates.get(key); it = list.iterator(); while
		 * (it.hasNext()) { System.out.println(it.next()); } }
		 */
	}

	// method to get surrogate list from a hashtable
	public ArrayList getSurrogateList(String key, int runStatus) {
		ArrayList list = new ArrayList();

		if (!surrogates.containsKey(key)) {
			writeLogFile("Error: Surrogate specification csv file does not contain surrogate -- " + key + LS, runStatus);
			return null;
		}
		list = (ArrayList) surrogates.get(key);
		return list;
	}

	public void readShapefiles() {
		String[] items = { "SHAPEFILE NAME", "DIRECTORY", "ELLIPSOID", "PROJECTION" }; // first field--key
		int keyItems = 1; // 1 = first item is the key
		// ArrayList list = new ArrayList();
		// Iterator it;

		/** get shpaefile information from shapefile catalog CSV file* */
		String fileName = getControls("SHAPEFILE CATALOG");
		checkFile(fileName, "NO", runStop); // check CSV fileName exists

		CSV file = new CSV(fileName);
		shapefiles = file.readCSV(items, keyItems);
		checkCSVError(shapefiles, runStop);

		/**
		 * System.out.println(LS+"\t\t"+"Shapefile Catalog CSV File"+LS+LS); ArrayList rows = (ArrayList)
		 * shapefiles.get(rowKey); for (int j=0;j<rows.size();j++) { String key = (String) rows.get(j);
		 * System.out.println("key: "+key); list = (ArrayList) shapefiles.get(key); it = list.iterator(); while
		 * (it.hasNext()) { System.out.println(it.next()); } }
		 */
	}

	// set item index in shapefile catalog
	public static final int DIRECTORY_INDEX = 0, ELLIPSOID_INDEX = 1, PROJECTION_INDEX = 2; // shapefile name is the key
																							// in the hashtable

	public String[] getShapeInfo(String shapeName, int runStatus) {
		ArrayList list = new ArrayList();
		String[] shapeData = new String[3]; // 0=DIRECTORY,1=ELLIPSOID,2=PROJECTION

		if (!shapefiles.containsKey(shapeName)) {
			writeLogFile("Error: Shapefile catalog CSV file does not contain shapefile -- " + shapeName + LS, runStatus);
			return null;
		}

		list = (ArrayList) shapefiles.get(shapeName);

		for (int i = DIRECTORY_INDEX; i <= PROJECTION_INDEX; i++) {
			shapeData[i] = (String) list.get(i); // put dir into array
			if ((shapeData[i].matches("^\\s*$")) || (shapeData[i].equals("NONE"))) {
				writeLogFile("Error: Shapefile catalog CSV file has missing value for " + shapeName + LS, runStatus);
				return null;
			}

		}

		// check main control file has SHAPEFILE DIRECTORY, if it does, use that directory instead
		if (controls.contains("SHAPEFILE DIRECTORY")) {
			shapeData[DIRECTORY_INDEX] = getControls("SHAPEFILE DIRECTORY");
		}

		// check exists of the shapefile
		if (!checkFile(shapeData[DIRECTORY_INDEX] + FS + shapeName + ".shp", "NO", runStatus)) {
			return null;
		}

		return shapeData;
	}

	// read data in from generation file and compare entries with specification file
	public void readGenerations() {
		String[] items = { "REGION", "SURROGATE CODE", "SURROGATE", "GENERATE", "QUALITY ASSURANCE" }; // REGION+SURROGATE
																										// CODE = key
																										// field
		int keyItems = 2; // 2 = combined first two items is the key
		ArrayList list = new ArrayList();
		ArrayList srgList = new ArrayList();

		String fileName = getControls("GENERATION CONTROL FILE");
		checkFile(fileName, "NO", runStop); // check CSV fileName exists

		CSV file = new CSV(fileName);
		generations = file.readCSV(items, keyItems);
		checkCSVError(generations, runStop);

		// compare entries with specification file and error checking YES and NO columns
		int checkStatus = 0;
		ArrayList rows = (ArrayList) generations.get(rowKey);
		for (int j = 0; j < rows.size(); j++) {
			String key = (String) rows.get(j);
			list = (ArrayList) generations.get(key);

			// check surrogate names
			if ((srgList = getSurrogateList(key, runError)) == null) {
				checkStatus = runError;
				continue;
			}

			String gSrgName = (String) list.get(SURROGATE_INDEX);
			String sSrgName = (String) srgList.get(SURROGATE_INDEX);
			if (!gSrgName.equals(sSrgName)) {
				writeLogFile("Error: Surrogate Names are different in the generation and specification files -- " + key
						+ LS, runError);
				checkStatus = runError;
			}

			// check YES and NO entries
			String generate = (String) list.get(GENERATE_INDEX);
			String g = generate.toUpperCase();
			if (!g.equals("YES") && (!g.equals("NO"))) {
				writeLogFile("Error: Wrong GENERATE entry in the generation file -- " + key + LS, runError);
				checkStatus = runError;
			}

			String quality = (String) list.get(QUALITY_ASSURANCE_INDEX);
			String q = quality.toUpperCase();
			if (!q.equals("YES") && (!q.equals("NO"))) {
				writeLogFile("Error: Wrong QUALITY ASSURANCE entry in the generation file -- " + key + LS, runError);
				checkStatus = runError;
			}
		}

		if (checkStatus == runError) {
			writeLogFile(LS, runStop); // if the file has any error, exit the run
		}
	}

	public void readSurrogateCodes() {
		String[] items = { "NAME", "#CODE" }; // first field--key field
		int keyItems = 1; // first item is the key
		// ArrayList list = new ArrayList();
		// Iterator it;

		String fileName = getControls("SURROGATE CODE FILE");
		checkFile(fileName, "NO", runStop); // check CSV fileName exists

		CSV file = new CSV(fileName);
		surrogateIDs = file.readCSV(items, keyItems);
		checkCSVError(surrogateIDs, runStop);

		/**
		 * System.out.println(LS+"\t\t"+"Surrogate IDs CSV File"+LS+LS); ArrayList rows = (ArrayList)
		 * surrogateIDs.get(rowKey); for (int j=0;j<rows.size();j++) { String key = (String) rows.get(j);
		 * System.out.println("key: "+key); list = (ArrayList) surrogateIDs.get(key); it = list.iterator(); while
		 * (it.hasNext()) { System.out.println(it.next()); } }
		 */
	}

	// method to get surrogate code from a hashtable
	public String getSurrogateCodes(String varName, int runStatus) {
		String value;
		ArrayList list = new ArrayList();
		int FIRST_ITEM = 0;
		int CODE_INDEX = 1;

		if (!surrogateIDs.containsKey(varName)) {
			writeLogFile("Error: Surrogate Code File does not contain name -- " + varName + LS, runStatus);
			return null;
		}
		list = (ArrayList) surrogateIDs.get(varName);
		value = (String) list.get(FIRST_ITEM);
		String[] code = value.split("=");
		if (code[CODE_INDEX].matches("^\\s*$")) {
			writeLogFile("Error: Surrogate Code File is missing the code for -- " + varName + " --" + LS, runStatus);
			return null;
		}
		return code[CODE_INDEX];
	}

	// set indexes for output poly shapefile information for headers
	public static final int OUTPUT_POLY_FILE_INDEX = 4, OUTPUT_POLY_ATTR_INDEX = 5, OUTPUT_FILE_ELLIPSOID_INDEX = 6,
			OUTPUT_FILE_MAP_PRJN_INDEX = 7; // output file info for polygon output type

	private void setMainVariables() {
		String[] env_gitems = { "DEBUG_OUTPUT", "OUTPUT_FORMAT", "OUTPUT_FILE_TYPE", "OUTPUT_GRID_NAME", "GRIDDESC",
				"OUTPUT_FILE_ELLIPSOID" }; // envs for grid output
		String[] env_pitems = { "DEBUG_OUTPUT", "OUTPUT_FORMAT", "OUTPUT_FILE_TYPE", "OUTPUT_POLY_FILE",
				"OUTPUT_POLY_ATTR" }; // envs for polygon output
		String[] env_egitems = { "DEBUG_OUTPUT", "OUTPUT_FORMAT", "OUTPUT_FILE_TYPE", "OUTPUT_GRID_NAME", "GRIDDESC",
				"OUTPUT_FILE_ELLIPSOID", "OUTPUT_POLY_FILE" }; // envs for grid output
		String[] shape = new String[3]; // store output polygon shapefile information

		// add library path to the execution
		if (OS.equals("unix"))
			mainEnv.add("LD_LIBRARY_PATH=" + LIBPATH);

		// create environment variable Vector for grid output format
		if (getControls("OUTPUT_FILE_TYPE").equals("RegularGrid")) {
			for (int i = 0; i < env_gitems.length; i++) {
				mainEnv.add(env_gitems[i] + "=" + getControls(env_gitems[i]));
			}
		} else if (getControls("OUTPUT_FILE_TYPE").equals("EGrid")) {
			for (int i = 0; i < env_egitems.length; i++) {
				mainEnv.add(env_egitems[i] + "=" + getControls(env_egitems[i]));
			}
		} else if (getControls("OUTPUT_FILE_TYPE").equals("Polygon")) { // /set POLYGON output format environmenta
																		// variables except last two
			for (int i = 0; i < env_pitems.length; i++) {
				mainEnv.add(env_pitems[i] + "=" + getControls(env_pitems[i]));
			}

			String oshape = getControls("OUTPUT_POLY_FILE"); // output polygon shapefile
			shape = getShapeInfo(oshape, runStop);
			String oPolyFile = "OUTPUT_POLY_FILE=" + shape[DIRECTORY_INDEX] + FS + oshape; // put polygon file with dir
																							// into array
			mainEnv.set(OUTPUT_POLY_FILE_INDEX, oPolyFile);
			mainEnv.add("OUTPUT_FILE_ELLIPSOID=" + shape[ELLIPSOID_INDEX]); // put output polygon ellipsoid in array
			mainEnv.add("OUTPUT_FILE_MAP_PRJN=" + "+" + shape[PROJECTION_INDEX]); // put polygon projection in array
		}

		// add hard coded environment variables
		mainEnv.add("DATA_FILE_NAME_TYPE=ShapeFile");
		mainEnv.add("WEIGHT_FILE_TYPE=ShapeFile");

		// set DENOMINATOR_THRESHOLD
		if (!controls.containsKey("DENOMINATOR_THRESHOLD")) {
			writeLogFile("Note: Main control CSV file does not contain variable -- DENOMINATOR_THRESHOLD" + LS,
					runContinue);
			writeLogFile("Note: DENOMINATOR_THRESHOLD will be set to the default threshold in srgcreate" + LS,
					runContinue);
		} else {
			mainEnv.add("DENOMINATOR_THRESHOLD=" + getControls("DENOMINATOR_THRESHOLD"));
		}

		System.out.println(LS + "\t\t" + "Main Environment Variables for SRGCREATE" + LS + LS);
		for (int i = 0; i < mainEnv.size(); i++) {
			System.out.println((String) mainEnv.get(i)); // print out main environment variables
		}
	}

	// copy main environment into a vector for a surrogate computation
	public Vector copyMainVar() {
		Vector all = new Vector();

		for (int i = 0; i < mainEnv.size(); i++) {
			all.add(mainEnv.get(i));
		}
		return all;
	}

	// return a filter txt file in temp_files directory under the output directory
	public String createFilterFile(String mainKey, String Dir, String header, String function) {
		String outFile;
		String key, value, line;
		String exp;
		String[] var;

		outFile = Dir + FS + "filter_" + mainKey + ".txt";

		if (!checkDir(Dir, runError) || !checkFile(outFile, "YES", runError)) {
			return null;
		}

		try {
			FileWriter fw = new FileWriter(outFile);
			BufferedWriter out = new BufferedWriter(fw);

			out.write(header + LS); // write the header
			out.write("# Filter Function: " + function + LS);
			String[] filters = function.split(";");

			for (int i = 0; i < filters.length; i++) {
				Pattern p = Pattern.compile("!=");
				Matcher m = p.matcher(filters[i]);
				Pattern pe = Pattern.compile("\\=");
				Matcher me = pe.matcher(filters[i]);
				System.out.println("Fiter Function: " + filters[i]);
				if (m.find()) {
					var = filters[i].split("!=", 2);
					exp = "EXCLUDE_VALUES=";
				} else if (me.find()) {
					var = filters[i].split("=", 2);
					exp = "INCLUDE_VALUES=";
				} else {
					writeLogFile("Error: " + mainKey + " has an invalid filter function -- " + filters[i] + LS,
							runError);
					return null;
				}
				key = var[0];
				value = var[1];
				line = "ATTRIBUTE_NAME=" + key + LS;
				out.write(line);
				p = Pattern.compile("([+-]?\\d*\\.?\\d*?)\\s*([-<>]=?)\\s*([+-]?\\d*\\.?\\d*?)");
				m = p.matcher(value);
				if (m.matches()) {
					line = "ATTRIBUTE_TYPE=" + "CONTINUOUS" + LS;
				} else {
					line = "ATTRIBUTE_TYPE=" + "DISCRETE" + LS;
				}
				out.write(line);
				line = exp + value + LS;
				out.write(line);
			}
			out.close();
		} catch (IOException e) {
			writeLogFile("Error -- " + e.toString() + LS, runError);
			return null;
		}

		return outFile;
	}

	// return surrogate merge txt file
	public String createMergeFile(String mainKey, String Dir, String header, String srgName, String function) {
		String outFile;
		String key, line;
		String num, srg, srgfile, file;

		// split mainKey into region and code
		String[] rc = mainKey.split("_"); // split partial function
		String region = rc[REGION_INDEX];
		// String code = rc[SURROGATE_CODE_INDEX];

		outFile = Dir + FS + "merge_" + mainKey + ".txt";
		if (!checkDir(Dir, runError) || !checkFile(outFile, "YES", runError)) {
			return null;
		}

		try {
			FileWriter fw = new FileWriter(outFile);
			BufferedWriter out = new BufferedWriter(fw);

			out.write(header + LS); // write the header
			out.write("# Merge Function: " + function + LS);
			if (getSurrogateCodes(srgName, runError) == null) {
				return null;
			}
			line = "OUTFILE=" + getControls("OUTPUT DIRECTORY") + FS + mainKey + NOFILL_TYPE + ".txt" + LS;
			out.write(line);
			line = "XREFFILE=" + getControls("SURROGATE CODE FILE") + LS;
			out.write(line);

			StringBuffer lines = new StringBuffer();
			String[] merges = function.split(";"); // multiple merge passes
			for (int j = 0; j < merges.length; j++) {
				float total_percent = 0; // total percent for checking, should add up to 1.0
				// different region merge
				Pattern rp = Pattern.compile("\\[.*\\]");
				Matcher rm = rp.matcher(merges[j]);

				// same region merge
				Pattern p = Pattern.compile("\\+");
				Matcher m = p.matcher(merges[j]);
				if (m.find()) {
					String[] srgs = merges[j].split("\\+"); // mutiple surrogates in merge function separated by "+"
					if (srgs.length != 2) {
						writeLogFile("Error: " + mainKey + " has more than 2 surrogates in the merge function -- " + merges[j] + LS,
								runError);
						return null;
					}
					for (int i = 0; i < srgs.length; i++) {
						String[] pf = srgs[i].split("\\*"); // split partial function
						if (pf.length != 2) {
							writeLogFile("Error: " + mainKey + " has an invalid merge function -- " + srgs[i] + LS,
									runError);
							return null;
						}
						// merge surrogates with specified percentages: 0.5*../data/surrogates|forest+0.5*Rural Land
						num = pf[0].trim(); // get percentage
						srgfile = pf[1].trim(); // get surrogate name
						float percent = Float.valueOf(num).floatValue(); // check percet of merge function
						if (percent < 0.0 || percent > 1.0) {
							writeLogFile("Error: " + mainKey + " has an invalid merging percentage -- " + srgs[i] + LS,
									runError);
							return null;
						}
						total_percent += percent;
						if (total_percent > 1.0) {
							writeLogFile("Error: " + mainKey + " has a total merging percetage > 1.0 -- " + merges[j]
									+ LS, runError);
							return null;
						}

						// handle external surrogate file merge
						Pattern pp = Pattern.compile("\\|");
						Matcher mm = pp.matcher(srgfile);
						if (mm.find()) {
							String[] es = srgfile.split("\\|"); // split surrogate file name by |
							if (es.length != 2) {
								writeLogFile("Error: " + mainKey + " has an invalid merge function -- " + srgs[i] + LS,
										runError);
								return null;
							}
							file = es[0].trim(); // get the name part before |
							srg = es[1].trim(); // get srg name after |
						} else {
							srg = srgfile;
							// get key for internal surrogate
							if (getSurrogateCodes(srg, runError) == null) {
								return null; // no code for the srg
							}
							key = getSurrogateCodes(srg, runError);
							file = getControls("OUTPUT DIRECTORY") + FS + region + "_" + key + NOFILL_TYPE + ".txt";
						}

						if (!checkFile(file, "NO", runError)) {
							writeLogFile("Error: Surrogate File Does Not Exist in Merge Function for: " + srgs[i] + LS,
									runError);
							return null; // no existence surrogate for the merge function
						}

						if (i == 0) {
							lines.append("OUTSRG=" + srgName + "; " + num + "*({" + file + "|" + srg + "})");
						} else {
							lines.append("+" + num + "*({" + file + "|" + srg + "})");
						}
						if (i == srgs.length - 1) {
							lines.append(LS); // add LS to the end of the function
						}
					}
				} else if (rm.find()) {
					// handle region combination: Population[USA]
					String[] srgs = merges[j].split("\\["); // surrogates and region in merge function separated by "["
					if (srgs.length != 2) {
						writeLogFile("Error: " + mainKey + " has an invalid merge function -- " + merges[j] + LS,
								runError);
						return null;
					}
					srg = srgs[0].trim();
					String[] rName = srgs[1].split("\\]"); // get region
					if (rName.length != 1) {
						writeLogFile("Error: " + mainKey + " has an invalid merge function -- " + merges[j] + LS,
								runError);
						return null;
					}
					String regionName = rName[0].trim(); // get region name
					// get key for surrogate
					if (getSurrogateCodes(srg, runError) == null) {
						return null; // no code for the srg
					}
					key = getSurrogateCodes(srg, runError);
					file = getControls("OUTPUT DIRECTORY") + FS + regionName + "_" + key + NOFILL_TYPE + ".txt";
					if (!checkFile(file, "NO", runError)) {
						writeLogFile("Error: Surrogate File Does Not Exist in Merge Function for: " + merges[j] + LS,
								runError);
						return null; // no existence surrogate for the merge function
					}
					num = "1.0"; // assign percentage
					// handle region combination
					lines.append("OUTSRG=" + srgName + "; " + num + "*{" + file + "|" + srg + "}" + LS);
				} else {
					writeLogFile("Error: " + mainKey + " has an invalid merge function -- " + merges[j] + LS, runError);
					return null;
				}
			}
			out.write(lines.toString());
			out.close();

		} catch (IOException e) {
			writeLogFile("Error -- " + e.toString() + LS, runError);
			return null;
		}

		return outFile;
	}

	// return surrogate gapfilling txt file
	public String createGapfillFile(String mainKey, String Dir, String header, String srgName, ArrayList srgs) {
		String outFile, srgFile;
		String key, line;
		String srg, file;

		// split mainKey into region and code
		String[] rc = mainKey.split("_"); // split partial function
		String region = rc[REGION_INDEX];
		// String code = rc[SURROGATE_CODE_INDEX];

		outFile = Dir + FS + "gapfill_" + mainKey + ".txt";

		if (!checkDir(Dir, runError) || !checkFile(outFile, "YES", runError)) {
			return null;
		}

		try {
			FileWriter fw = new FileWriter(outFile);
			BufferedWriter out = new BufferedWriter(fw);

			out.write(header + LS); // write the header
			line = "OUTFILE=" + getControls("OUTPUT DIRECTORY") + FS + mainKey + FILL_TYPE + ".txt" + LS;
			out.write(line);
			line = "XREFFILE=" + getControls("SURROGATE CODE FILE") + LS;
			out.write(line);

			line = new String();
			if (!checkFile(srgFile = getControls("OUTPUT DIRECTORY") + FS + mainKey + NOFILL_TYPE + ".txt", "NO",
					runError)) {
				// check main surrogate file
				return null;
			}
			line = "GAPFILL=" + srgFile + "|" + srgName;
			for (int i = 0; i < srgs.size(); i++) {
				// handle external surrogate file merge
				String sFile = (String) srgs.get(i);
				Pattern p = Pattern.compile("\\|");
				Matcher m = p.matcher(sFile);
				if (m.find()) {
					String[] es = sFile.split("\\|"); // split surrogate file name by |
					if (es.length != 2) {
						writeLogFile("Error: Wrong Gapfill Function for: " + sFile + LS, runError);
						return null;
					}
					file = es[0].trim(); // get the name part before |: can be real file
					srg = es[1].trim(); // get srg name after |
				} else {
					srg = sFile;
					// get key for internal surrogate
					if (getSurrogateCodes(srg, runError) == null) {
						return null; // no code for the srg
					}
					key = getSurrogateCodes(srg, runError);
					file = getControls("OUTPUT DIRECTORY") + FS + region + "_" + key + NOFILL_TYPE + ".txt";
				}

				if (!checkFile(file, "NO", runError)) {
					writeLogFile("Error: Surrogate File Does Not Exist in Gapfill Function for: " + sFile + LS,
							runError);
					return null; // no existence surrogate for the gapfill function
				}

				line = line + ";" + file + "|" + srg;
			}
			line = "OUTSRG=" + srgName + "; " + line + LS;
			out.write(line);
			out.close();

		} catch (IOException e) {
			writeLogFile("Error -- " + e.toString() + LS, 9);
			return null;
		}

		return outFile;

	}

	// output environment variables to a batch or csh file
	public String writeFile(String dir, String tfile, String header, Vector env, String exe) {
		String line, key, value;
		String outFile;

		if (!checkDir(dir, runError)) {
			return null;
		}

		if (OS.equals("window")) {
			outFile = dir + FS + tfile + ".bat";
		} else {
			// assume unix
			outFile = dir + FS + tfile + ".csh";
		}

		try {
			FileWriter fw = new FileWriter(outFile);
			BufferedWriter out = new BufferedWriter(fw);

			out.write(SCRIPT_HEADER + LS); // write the header for the scripts file
			out.write(COMMENT + header + LS); // write the header for surrogate generation
			for (int i = 0; i < env.size(); i++) {
				String all = (String) env.get(i);
				String[] var = all.split("=", 2);
				key = var[0];
				value = var[1];
				line = CMD + " " + key + EQ + value + LS;
				System.out.println("CMD line:" + line);
				out.write(line);
			}

			// run the program
			line = TIME + LS;
			out.write(line);
			line = exe + LS;
			out.write(line);
			out.close();

		} catch (IOException e) {
			writeLogFile("Error -- " + e.toString() + LS, 7);
			return null;
		}
		return outFile;
	}

	// get log list for a surrogate
	public ArrayList getSrgLogList(String key, String srgName) {
		ArrayList logList = new ArrayList();

		// System.out.println("get SrgLogList key: "+key+" srgName: "+srgName+LS);
		if (runLog.containsKey(key)) {
			logList = (ArrayList) runLog.get(key);
		} else {
			logList.add(srgName);
		}
		return logList;
	}

	// update run log list for the surrogate
	public void putSrgRunLog(String key, ArrayList list, String message) {

		if (runLog.containsKey(key)) {
			runLog.remove(key);
		}

		// System.out.println(LS+"Put SrgLogList key: "+key+" message: "+message+LS);
		list.add(message);
		runLog.put(key, list);
	}

	public boolean checkRunMessage(String message) {

		Pattern p = Pattern.compile("ERROR IN RUNNING THE");
		Matcher m = p.matcher(message);
		if (m.find()) {
			writeLogFile(message + LS, runError);
			return true;
		}
		writeLogFile(message + LS, runContinue);
		return false;
	}

	// get grid, egrid or polygon header information
	private void getGridPolyHeader() {
		Vector allVar = new Vector();
		// String[] cmd = new String[4]; //has an argument--header

		if (getControls("OUTPUT_FILE_TYPE").equals("RegularGrid") || getControls("OUTPUT_FILE_TYPE").equals("EGrid")) { // get
																														// grid
																														// information
			System.out.println("\t\t" + "Get Grid Header For Surrogate Files");
			writeLogFile(LS + "\t\t" + "Get Grid Header For Surrogate Files" + LS, runContinue);
			allVar = copyMainVar(); // copy main env to a vector
			String[] env = new String[allVar.size()]; // put all environment variables in a String array
			for (int i = 0; i < allVar.size(); i++) {
				env[i] = (String) allVar.get(i);
			}

			COMMAND[2] = getControls("SRGCREATE EXECUTABLE") + "  " + "-header";
			// System.arraycopy(COMMAND,0,cmd,0,COMMAND.length);
			// cmd[3] = "-header";
			RunScripts rs = new RunScripts("SRGCREATE", COMMAND, env);
			String runMessage = rs.run();
			Pattern p = Pattern.compile("#GRID(.*)");
			Matcher m = p.matcher(runMessage);
			if (checkRunMessage(runMessage)) {
				writeLogFile("Error: Getting Grid Header Failed, check grid description file." + LS, runStop);
				System.out.println("Error: Getting grid header failed, check grid description file." + LS);
			} else if (m.find()) {
				GRIDPOLY_HEADER = m.group() + LS; // get matched header with the end of line
				System.out.println(GRIDPOLY_HEADER);
			} else {
				writeLogFile("Error: No Grid Header from the SRGCREATE Run Output." + LS, runStop);
				System.out.println("Error: No Grid Header from the SRGCREATE Run Output." + LS);
			}
		} else if (getControls("OUTPUT_FILE_TYPE").equals("Polygon")) { // set output polygon information
			GRIDPOLY_HEADER = "#POLYGON\t" + mainEnv.get(OUTPUT_POLY_FILE_INDEX) + "\t"
					+ mainEnv.get(OUTPUT_POLY_ATTR_INDEX) + "\t" + mainEnv.get(OUTPUT_FILE_ELLIPSOID_INDEX) + "\t"
					+ mainEnv.get(OUTPUT_FILE_MAP_PRJN_INDEX) + LS;
			writeLogFile(LS + "Header of The Surrogate Ratio File" + LS + GRIDPOLY_HEADER + LS, runContinue);
			System.out.println(GRIDPOLY_HEADER);
		}
	}

	private boolean renameFile(String oldFile, String newFile, int runStatus) {

		// File (or directory) with old name
		File file = new File(oldFile);

		// File (or directory) with new name
		File file2 = new File(newFile);

		// Rename file (or directory)
		boolean success = file.renameTo(file2);
		if (!success) {
			writeLogFile("Error: In Renaming File: " + oldFile + " to " + newFile + LS, runStatus);
			return false;
		}

		return success;
	}

	private boolean addSrgHeaders(String key, String fileName, int runExe) {

		StringBuffer headers = new StringBuffer();
		ArrayList list = new ArrayList();
		String line;

		headers.append(GRIDPOLY_HEADER);

		if ((list = getSurrogateList(key, runError)) == null) {
			writeLogFile("Error: No surrogate specification data for " + key + LS, runError);
			return false;
		}

		headers.append("#SRGDESC=" + (String) list.get(SURROGATE_CODE_INDEX) + "," + (String) list.get(SURROGATE_INDEX)
				+ LS);
		headers.append("#" + LS);
		headers.append("#SURROGATE REGION = " + (String) list.get(REGION_INDEX) + LS);
		headers.append("#SURROGATE CODE = " + (String) list.get(SURROGATE_CODE_INDEX) + LS);
		headers.append("#SURROGATE NAME = " + (String) list.get(SURROGATE_INDEX) + LS);

		if (runExe == CREATE_STATUS_INDEX || runExe == GAPFILL_STATUS_INDEX) {
			headers.append("#DATA SHAPEFILE = " + (String) list.get(DATA_SHAPEFILE_INDEX) + LS);
			headers.append("#DATA ATTRIBUTE = " + (String) list.get(DATA_ATTRIBUTE_INDEX) + LS);
			headers.append("#WEIGHT SHAPEFILE = " + (String) list.get(WEIGHT_SHAPEFILE_INDEX) + LS);
			headers.append("#WEIGHT ATTRIBUTE = " + (String) list.get(WEIGHT_ATTRIBUTE_INDEX) + LS);
			headers.append("#WEIGHT FUNCTION = " + (String) list.get(WEIGHT_FUNCTION_INDEX) + LS);
			headers.append("#FILTER FUNCTION = " + (String) list.get(FILTER_FUNCTION_INDEX) + LS);
		}

		if (runExe == MERGE_STATUS_INDEX) {
			headers.append("#MERGE FUNCTION = " + (String) list.get(MERGE_FUNCTION_INDEX) + LS);
		}

		if (runExe == GAPFILL_STATUS_INDEX) {
			headers.append("#MERGE FUNCTION = " + (String) list.get(MERGE_FUNCTION_INDEX) + LS);
			headers.append("#SECONDARY SURROGATE = " + (String) list.get(SECONDARY_SURROGATE_INDEX) + LS);
			headers.append("#TERTIARY SURROGATE = " + (String) list.get(TERTIARY_SURROGATE_INDEX) + LS);
			headers.append("#QUARTERNARY SURROGATE = " + (String) list.get(QUARTERNARY_SURROGATE_INDEX) + LS);
		}

		// add csv files to headers
		headers.append("#" + LS);
		headers.append("#CONTROL VARIABLE FILE = " + CONTROL_VARIABLE_FILE + LS);
		headers.append("#SURROGATE SPECIFICATION FILE = " + getControls("SURROGATE SPECIFICATION FILE") + LS);
		headers.append("#SHAPEFILE CATALOG = " + getControls("SHAPEFILE CATALOG") + LS);
		headers.append("#GENERATION CONTROL FILE = " + getControls("GENERATION CONTROL FILE") + LS);
		headers.append("#SURROGATE CODE FILE = " + getControls("SURROGATE CODE FILE") + LS);

		String outputFType = getControls("OUTPUT_FILE_TYPE");
		if (outputFType.equals("RegularGrid") || outputFType.equals("EGrid")) {
			headers.append("#GRIDDESC = " + getControls("GRIDDESC") + LS);
			headers.append("#OUTPUT_FILE_ELLIPSOID = " + getControls("OUTPUT_FILE_ELLIPSOID") + LS);
			if (outputFType.equals("EGrid")) {
				headers.append("#OUTPUT_POLY_FILE in ArcGIS polygon text format = " + getControls("OUTPUT_POLY_FILE")
						+ LS);
			}
		}
		if (outputFType.equals("Polygon")) {
			headers.append("#OUTPUT_POLY_FILE = " + getControls("OUTPUT_POLY_FILE") + LS);
			headers.append("#OUTPUT_POLY_ATTR = " + getControls("OUTPUT_POLY_ATTR") + LS);
		}

		headers.append("#DENOMINATOR_THRESHOLD = " + getControls("DENOMINATOR_THRESHOLD") + LS);

		// add user and time
		headers.append("#" + LS);
		headers.append("#USER = " + USER + LS);
		headers.append("#COMPUTER SYSTEM = " + CS + LS);
		Date now = new Date();
		headers.append("#DATE = " + now.toString() + LS);

		// temp file
		String newFile = fileName + ".tmp";

		checkFile(fileName, "NO", runStop);
		if (!renameFile(fileName, newFile, runError)) {
			return false;
		}

		try {
			// open input file
			FileReader fr = new FileReader(newFile);
			BufferedReader buff = new BufferedReader(fr);

			// open output file
			FileWriter fw = new FileWriter(fileName);
			BufferedWriter out = new BufferedWriter(fw);
			// write headers
			out.write(headers.toString());

			// read the input file
			while ((line = buff.readLine()) != null) {
				// skip lines starting with # or empty lines
				if (line.matches("^\\#GRID.*") || line.matches("^\\#GRID\\t.*") ||
					line.matches("^\\#POLYGON.*") || line.matches("^\\#POLYGON\\t.*") || 	
					line.matches("^\\s*$") || line.matches("^\\#SRGDESC.*")) {
					continue;
				}
				out.write(line + LS);
			}
			out.close();
			buff.close();

		} catch (IOException e) {
			writeLogFile("Error -- " + e.toString() + LS, runError);
			return false;
		}

		// delete the temp file
		boolean success = (new File(newFile)).delete();
		if (!success) {
			writeLogFile("Warning: In Deleting Temp File: " + newFile + LS, runError);
		}

		return true;
	}

	// add output surrogate file information into a hashtable
	private void addSrgDesc(String key, String srgName, String fileName) {

		// split key into region and code
		String[] rc = key.split("_"); // split partial function
		String region = rc[REGION_INDEX];
		String code = rc[SURROGATE_CODE_INDEX];

		// delete the existing entry
		if (srgDesc.containsKey(key)) {
			srgDesc.remove(key);
		}

		// add new entry
		String line = region + "," + code + ",\"" + srgName + "\"," + fileName;
		System.out.println("srgDesc Line = " + line);
		srgDesc.put(key, line);
	}

	// write SRGDESC information into a file and merge all individual srg files if users want
	private void writeSrgDesc() {
		String line; // one record from csv file
		List list = new ArrayList();
		int ITEMS = 4;
		int SRGFILE_INDEX = 3;
		String outSrgTotal;

		// return if no new surrogate files created
		if (srgDesc.isEmpty()) {
			return;
		}

		// get SRGDESC file and check the existence
		String fileName = getControls("OUTPUT SRGDESC FILE");
		CSV csv = new CSV(); // for parsing input csv file line

		if (checkFile(fileName, "NONE", runContinue)) {
			try {
				FileReader file = new FileReader(fileName);
				BufferedReader buff = new BufferedReader(file);

				while ((line = buff.readLine()) != null) {
					// get rid of headers
					if (line.matches("^\\#.*") || line.matches("^\\s*$")) {
						continue;
					}
					list = csv.parse(line.trim());
					// check total items in each line
					if (list.size() != ITEMS) {
						writeLogFile("Warning: Wrong format line in " + fileName + ": " + line + LS, runError);
					} else {
						String region = (String) list.get(REGION_INDEX);
						String code = (String) list.get(SURROGATE_CODE_INDEX);
						String srgName = (String) list.get(SURROGATE_INDEX);
						String srgFile = (String) list.get(SRGFILE_INDEX);

						String key = region + "_" + code;
						if (srgDesc.containsKey(key)) {
							continue; // skip old file and replace it with the new file
						}
						String newline = region + "," + code + ",\"" + srgName + "\"," + srgFile;
						srgDesc.put(key, newline);
					}
				}
				buff.close();
			} catch (IOException e) {
				writeLogFile("Error: Reading existing OUTPUT SRGDESC FILE --" + e.toString() + LS, runError);
			}

			// delete the old srgdesc file
			if (!checkFile(fileName, "YES", runError)) {
				writeLogFile("Error: Deleting existing OUTPUT SRGDESC FILE -- " + fileName + LS, runError);
			}
			System.out.println("Finished checking the SRGDESC file");
		}

		// write out srgdesc information to the file and append individual srg files based on the srgdesc information
		// check the existence of appened srg output file
		String ow = getControls("OVERWRITE OUTPUT FILES");
		String OW = ow.toUpperCase();
		outSrgTotal = getControls("OUTPUT SURROGATE FILE");
		if (!outSrgTotal.equals("NONE")) {
			if (OW.equals("YES")) {
				if (!checkFile(outSrgTotal, "YES", runError)) {
					writeLogFile("Error: Deleting existing OUTPUT SURROGATE FILE -- " + outSrgTotal + LS, runError);
					OW = "NO"; // no new OUTPUT SURROGATE FILE
				}
			} else {
				if (!checkFile(outSrgTotal, "NONE", runContinue)) {
					OW = "YES"; // output total srg file if no existing old file
				}
			}
		} else {
			OW = "NO";
		}

		try {
			// open output SRGDESC file
			FileWriter fw = new FileWriter(fileName); // overwrite the existing file whether or not it is deleted
			BufferedWriter out = new BufferedWriter(fw);

			// write grid headers
			out.write(GRIDPOLY_HEADER);

			// output srgdesc data from the sorted hashtable
			Vector v = new Vector(srgDesc.keySet());
			Collections.sort(v);
			Iterator it = v.iterator();
			while (it.hasNext()) {
				String key = (String) it.next();
				line = (String) srgDesc.get(key);
				// System.out.println("srgDesc Line = "+line);
				out.write(line + LS);

				list = csv.parse(line.trim());
				// String region = (String) list.get(REGION_INDEX);
				// String code = (String) list.get(SURROGATE_CODE_INDEX);
				// String srgName = (String) list.get(SURROGATE_INDEX);
				String srgFile = (String) list.get(SRGFILE_INDEX);

				if (OW.equals("YES") && checkFile(srgFile, "NONE", runContinue)) {
					try {
						// open total srg file for writing
						FileWriter fwSrg = new FileWriter(outSrgTotal, true); // append all files together
						BufferedWriter outSrg = new BufferedWriter(fwSrg);

						// open individual srg file for reading
						FileReader file = new FileReader(srgFile);
						BufferedReader buff = new BufferedReader(file);

						while ((line = buff.readLine()) != null) {
							outSrg.write(line + LS);
						}
						outSrg.close();
						buff.close();
					} catch (IOException e) {
						writeLogFile("Error: In reading surrogate file for appending --" + line + LS, runError);
					}
				} else if (!checkFile(srgFile, "NONE", runContinue)) {
					writeLogFile("Error: Surrogate file does not exist in SRGDESC file: " + line + LS, runError);
				}
			}
			out.close();
		} catch (IOException e) {
			writeLogFile("Error: Writing OUTPUT SRGDESC FILE --" + e.toString() + LS, runError);
		}
		System.out.println("Finished writing srgDesc file and total surrogate file");
	}

	// set item index in surrogate specification file and surrogate generation file
	// REGION+SURROGATE CODE = key
	public static final int REGION_INDEX = 0, SURROGATE_CODE_INDEX = 1, SURROGATE_INDEX = 2, DATA_SHAPEFILE_INDEX = 3,
			DATA_ATTRIBUTE_INDEX = 4, WEIGHT_SHAPEFILE_INDEX = 5, WEIGHT_ATTRIBUTE_INDEX = 6,
			WEIGHT_FUNCTION_INDEX = 7, FILTER_FUNCTION_INDEX = 8, MERGE_FUNCTION_INDEX = 9,
			SECONDARY_SURROGATE_INDEX = 10, TERTIARY_SURROGATE_INDEX = 11, QUARTERNARY_SURROGATE_INDEX = 12;

	public static final int GENERATE_INDEX = 3, QUALITY_ASSURANCE_INDEX = 4;

	public static final int CREATE_STATUS_INDEX = 5, MERGE_STATUS_INDEX = 6, GAPFILL_STATUS_INDEX = 7; // processing
																										// status stored
																										// in
																										// generations
																										// table

	// set item index in surrogate code file
	public static final int CODE_INDEX = 0; // name is the key in the hashtable

	private void generateSurrogates() {
		/**
		 * loop through and compute the surrogates without merge and gapfilling functions. 1. set environmental
		 * variables for the surrogate 2. run srgcreate.exe
		 */

		ArrayList list = new ArrayList();
		ArrayList srgList = new ArrayList();
		Vector allVar = new Vector();
		String header;
		String outDir;
		String srgFile;
		String[] shape = new String[3];

		// put variable in a list or vector first
		if (!checkDir(outDir = getControls("OUTPUT DIRECTORY"), runError)) {
			return; // srgcreate ends
		}

		String ow = getControls("OVERWRITE OUTPUT FILES");
		String OW = ow.toUpperCase();

		ArrayList rows = (ArrayList) generations.get(rowKey);
		for (int j = 0; j < rows.size(); j++) {
			String key = (String) rows.get(j);
			list = (ArrayList) generations.get(key);
			String generate = (String) list.get(GENERATE_INDEX);
			String g = generate.toUpperCase();

			// create log list for each surrogate
			ArrayList logList = getSrgLogList(key, (String) list.get(SURROGATE_INDEX));
			// System.out.println("KEY: "+key + "Generate: "+g);

			if ((srgList = getSurrogateList(key, runError)) == null) {
				putSrgRunLog(key, logList, "Failed: No specification data");
				continue;
			}

			String wshape = (String) srgList.get(WEIGHT_SHAPEFILE_INDEX);
			String wa = (String) srgList.get(WEIGHT_ATTRIBUTE_INDEX); // weight attribute
			String wf = (String) srgList.get(WEIGHT_FUNCTION_INDEX); // weight function
			String ff = (String) srgList.get(FILTER_FUNCTION_INDEX); // filter function
			// System.out.println("Weight Shape: "+(String) srgList.get(SHAPEFILE_INDEX)+" "+wa+" "+wf+" "+ff);

			if (g.equals("YES")
					&& ((!wshape.matches("^\\s*$")) || (!wa.matches("^\\s*$")) || (!wf.matches("^\\s*$")) || (!ff
							.matches("^\\s*$")))) {
				System.out.println(LS + "Run srgcreate.exe to generate surrogate ratios for " + key + ": "
						+ (String) list.get(SURROGATE_INDEX));
				writeLogFile(LS + "Run srgcreate.exe to generate surrogate ratios for " + key + ": "
						+ (String) list.get(SURROGATE_INDEX) + LS, runContinue);
				if (checkFile(srgFile = outDir + FS + key + NOFILL_TYPE + ".txt", "NONE", runContinue)
						&& OW.equals("NO")) {
					// the output file exists and skip the surrogate
					writeLogFile(LS + "Surrogate ratio file exists for " + key + ": "
							+ (String) list.get(SURROGATE_INDEX) + LS, runContinue);
					putSrgRunLog(key, logList, "SRGCREATE Skipped");
					continue;
				}

				allVar = copyMainVar(); // copy main env to the current surrogate
				allVar.add("WRITE_HEADER=NO");

				// set data (emission data base) shapefile environments
				String dshape = (String) srgList.get(DATA_SHAPEFILE_INDEX); // data shapefile
				if ((shape = getShapeInfo(dshape, runError)) == null) {
					putSrgRunLog(key, logList, "SRGCREATE Failed");
					continue;
				}
				;
				allVar.add("DATA_FILE_NAME=" + shape[DIRECTORY_INDEX] + FS + dshape); // put datafile with dir into
																						// array
				allVar.add("DATA_FILE_ELLIPSOID=" + shape[ELLIPSOID_INDEX]); // put ellipsoid of data file in array
				allVar.add("DATA_FILE_MAP_PRJN=" + "+" + shape[PROJECTION_INDEX]); // put projection of data file in
																					// array
				String da = (String) srgList.get(DATA_ATTRIBUTE_INDEX); // data shapefile attribute
				if (da.matches("^\\s*$")) {
					writeLogFile("ERROR: DATA ATTRIBUTE is not specified for " + (String) list.get(SURROGATE_INDEX)
							+ LS, runError);
					putSrgRunLog(key, logList, "SRGCREATE Failed");
					continue;
				}
				allVar.add("DATA_ID_ATTR=" + (String) srgList.get(DATA_ATTRIBUTE_INDEX));

				// set quality controls
				String Qa = (String) list.get(QUALITY_ASSURANCE_INDEX);
				String q = Qa.toUpperCase();
				if (!q.equals("YES")) {
					q = "NO";
				}
				allVar.add("WRITE_QASUM=" + q);
				allVar.add("WRITE_SRG_NUMERATOR=" + q);
				allVar.add("WRITE_SRG_DENOMINATOR=" + q);

				allVar.add("SURROGATE_ID=" + (String) list.get(SURROGATE_CODE_INDEX));
				allVar.add("SURROGATE_FILE=" + srgFile);

				// set weight shapefile
				if ((shape = getShapeInfo(wshape, runError)) == null) {
					putSrgRunLog(key, logList, "SRGCREATE Failed");
					continue;
				}
				;
				allVar.add("WEIGHT_FILE_NAME=" + shape[DIRECTORY_INDEX] + FS + wshape);
				allVar.add("WEIGHT_FILE_ELLIPSOID=" + shape[ELLIPSOID_INDEX]); // put ellipsoid of data file in array
				allVar.add("WEIGHT_FILE_MAP_PRJN=" + "+" + shape[PROJECTION_INDEX]); // put projection of data file
																						// in array

				// handle weight attribute and function
				if ((!wf.matches("^\\s*$")) && (!wf.equals("NONE"))) {
					allVar.add("WEIGHT_FUNCTION=" + wf);
					allVar.add("WEIGHT_ATTR_LIST=" + "USE_FUNCTION");
				} else {
					allVar.add("WEIGHT_FUNCTION=NONE");
					allVar.add("WEIGHT_ATTR_LIST=" + wa);
				}

				// handle filter function
				if ((!ff.matches("^\\s*$")) && (!ff.equals("NONE"))) {
					header = "# Filter txt file for surrogate -- " + key + "=" + (String) list.get(SURROGATE_INDEX);
					String filterFile = createFilterFile(key, outDir + FS + tDir, header, ff);
					if (filterFile == null) {
						putSrgRunLog(key, logList, "Failed: creating filter txt file");
						continue;
					}
					allVar.add("FILTER_FILE=" + filterFile);
					allVar.add("FILTERED_WEIGHT_SHAPES=" + outDir + FS + "filtered_" + wshape + "_" + key);
				} else {
					allVar.add("FILTER_FILE=NONE");
				}

				String outputFType = getControls("OUTPUT_FILE_TYPE");

				if (outputFType.equals("RegularGrid")) {
					allVar.add("OUTPUT_FILE_NAME=" + outDir + FS + "grid_" + key); // output surrogate grid shapefile
				} else if (outputFType.equals("EGrid")) {
					allVar.add("OUTPUT_FILE_NAME=" + outDir + FS + "egrid_" + key); // output surrogate grid shapefile
				} else if (outputFType.equals("Polygon")) {
					allVar.add("OUTPUT_FILE_NAME=" + outDir + FS + "poly_" + key); // output surrogate grid shapefile
				}

				// write all environment variables to a file in temp_files directory under OUTPUT DIRECTORY
				header = "Environment variables for computation of surrogate -- " + key + "="
						+ (String) list.get(SURROGATE_INDEX);
				String scripts = writeFile(outDir + FS + tDir, key + NOFILL_TYPE, header, allVar,
						getControls("SRGCREATE EXECUTABLE"));
				if (scripts == null) {
					logList.add("Warning: failed to create bat or csh file");
				}

				String[] env = new String[allVar.size()]; // put all environment variables in a String array
				System.out.println("\t\t" + "Environment Variables Settings for SRGCREATE" + LS);
				for (int i = 0; i < allVar.size(); i++) {
					env[i] = (String) allVar.get(i);
					System.out.println((String) allVar.get(i));
				}

				COMMAND[2] = getControls("SRGCREATE EXECUTABLE");
				RunScripts rs = new RunScripts("SRGCREATE", COMMAND, env);
				String runMessage = rs.run();
				rs = null; // free all memory used by rs
				if (checkRunMessage(runMessage)) {
					putSrgRunLog(key, logList, "SRGCREATE Failed");
					System.out.println(key + "   SRGCREATE Failed");
				} else {
					putSrgRunLog(key, logList, "SRGCREATE Success");
					System.out.println(LS + key + "   SRGCREATE Success" + LS);

					if (!addSrgHeaders(key, srgFile, CREATE_STATUS_INDEX)) {
						putSrgRunLog(key, logList, "SRGCREATE Headers Failed");
						System.out.println(LS + key + "   SRGCREATE Headers Failed" + LS);
					} else {
						addSrgDesc(key, (String) list.get(SURROGATE_INDEX), srgFile);
						writeSrgDesc();
					}
				}
			}
		}
	}

	/**
	 * Loop through the surrogates to use srgmerge.exe or Java merging tool for merging Set environmental variables for
	 * the run and create temp file txt for merge Run srgmerge.exe or merge tool.
	 */
	private void mergeSurrogates() {
		ArrayList list = new ArrayList();
		String header;
		String outDir;
		String srgFile;
		String[] cmd = new String[4]; // has an argument--merge txt file
		String[] env = new String[0];
		String runMessage;

		if (!checkDir(outDir = getControls("OUTPUT DIRECTORY"), runError)) {
			return; // srgmerge ends
		}

		String ow = getControls("OVERWRITE OUTPUT FILES");
		String OW = ow.toUpperCase();

		ArrayList rows = (ArrayList) generations.get(rowKey);
		for (int j = 0; j < rows.size(); j++) {
			String key = (String) rows.get(j);
			list = (ArrayList) generations.get(key);
			String generate = (String) list.get(GENERATE_INDEX);
			String g = generate.toUpperCase();

			// create log list for each surrogate
			ArrayList logList = getSrgLogList(key, (String) list.get(SURROGATE_INDEX));
			// System.out.println("KEY: "+key);

			ArrayList srgList = getSurrogateList(key, runError);
			if (srgList == null) {
				putSrgRunLog(key, logList, "Failed: No specification data");
				continue;
			}

			String mf = (String) srgList.get(MERGE_FUNCTION_INDEX); // merge function

			if (g.equals("YES") && (!mf.matches("^\\s*$")) && (!mf.equals("NONE"))) {

				System.out.println(LS + "Merging surrogate ratios for " + key + ": "
						+ (String) list.get(SURROGATE_INDEX));
				writeLogFile(LS + "Merging surrogate ratios for " + key + ": " + (String) list.get(SURROGATE_INDEX)
						+ LS, runContinue);

				if (checkFile(srgFile = outDir + FS + key + NOFILL_TYPE + ".txt", "NONE", runContinue)
						&& OW.equals("NO")) {
					// the output file exists and skip the surrogate
					writeLogFile(LS + "Surrogate ratio file exists for " + key + ": "
							+ (String) list.get(SURROGATE_INDEX) + LS, runContinue);
					putSrgRunLog(key, logList, "MERGING Skipped");
					continue;
				}

				// create merge txt file
				header = "# Merging surrogate ratios to generate surrogate -- " + key + "="
						+ (String) list.get(SURROGATE_INDEX);
				String mergeFile = createMergeFile(key, outDir + FS + tDir, header, (String) list.get(SURROGATE_INDEX),
						mf);
				if (mergeFile == null) {
					System.out.println("Creating MERGE TXT File failed");
					putSrgRunLog(key, logList, "Creating MERGE TXT File Failed");
					continue;
				}
				if (hasSrgMerge) {
					COMMAND[2] = getControls("SRGMERGE EXECUTABLE") + "   " + mergeFile;

					System.arraycopy(COMMAND, 0, cmd, 0, COMMAND.length);
					cmd[3] = mergeFile;
					RunScripts rs = new RunScripts("SRGMERGE", COMMAND, env);
					runMessage = rs.run();
					rs = null; // free all memory used by rs
				} else {
					new SystemInfo().print();
					try {
						Merging mg = new Merging(mergeFile);
						mg.doMerging();
						runMessage = "SUCCESS RUNNING THE JAVA MERGE TOOL";
					} catch (Exception e) {
						System.out.println(e.getMessage());
						runMessage = "ERROR RUNNING THE JAVA MERGE TOOL";
					}
				}
				if (checkRunMessage(runMessage)) {
					putSrgRunLog(key, logList, "MERGING Failed");
					System.out.println(key + "  MERGING Failed");
				} else {
					putSrgRunLog(key, logList, "MERGING Success");
					System.out.println(key + "  MERGING Success");
					if (!addSrgHeaders(key, srgFile, MERGE_STATUS_INDEX)) {
						putSrgRunLog(key, logList, "MERGING Headers Failed");
						System.out.println(LS + key + "   MERGING Headers Failed" + LS);
					} else {
						addSrgDesc(key, (String) list.get(SURROGATE_INDEX), srgFile);
						writeSrgDesc();
					}
				} // end else
			} // end of merging surrogates
		} // end of j
	} // end of the method

	// loop through the surrogates to use srgmerge.exe or Java gapfill tool for gapfilling
	// set environmental variables for the run
	// run srgmerge.exe or Java gafill tool
	private void gapfillSurrogates() {
		ArrayList list = new ArrayList();
		// Iterator it;
		String header;
		String outDir;
		String srgFile;
		String[] cmd = new String[4]; // has an argument--merge txt file
		String[] env = new String[0];
		String runMessage;

		if (!checkDir(outDir = getControls("OUTPUT DIRECTORY"), runError)) {
			return; // srgmerge for gapfilling ends
		}

		String ow = getControls("OVERWRITE OUTPUT FILES");
		String OW = ow.toUpperCase();

		ArrayList rows = (ArrayList) generations.get(rowKey);
		for (int j = 0; j < rows.size(); j++) {
			String key = (String) rows.get(j);
			list = (ArrayList) generations.get(key);
			String generate = (String) list.get(GENERATE_INDEX);
			String g = generate.toUpperCase();

			// create log list for each surrogate
			ArrayList logList = getSrgLogList(key, (String) list.get(SURROGATE_INDEX));
			// System.out.println("KEY: "+key);

			ArrayList srgList = getSurrogateList(key, runError);
			if (srgList == null) {
				putSrgRunLog(key, logList, "Failed: No Specification Data");
				continue;
			}

			ArrayList srgs = new ArrayList();
			int[] gfs = new int[] { 0, 0, 0 }; // array for checking sequence of the gapfilling surrogates
			boolean gapfill = false;
			for (int i = SECONDARY_SURROGATE_INDEX; i <= QUARTERNARY_SURROGATE_INDEX; i++) {
				String srg = (String) srgList.get(i); // gapfilling file
				if (!srg.matches("^\\s*$") && (!srg.equals("NONE"))) {
					srgs.add(srg);
					gfs[i - SECONDARY_SURROGATE_INDEX] = i;
					gapfill = true;
				}
			}

			if (gfs[0] == 0 && (gfs[1] != 0 || gfs[2] != 0)) {
				writeLogFile(LS + "SECONDARY SURROGATE is not specified first for " + key + ": "
						+ (String) list.get(SURROGATE_INDEX) + LS, runError);
				putSrgRunLog(key, logList, "Failed: Invalid Gapfilling Surrogate Sequence");
				continue;
			}

			if (gfs[1] == 0 && gfs[2] != 0) {
				writeLogFile(LS + "TERTIARY SURROGATE is not specified before QUARTERNARY SURROGATE for " + key + ": "
						+ (String) list.get(SURROGATE_INDEX) + LS, runError);
				putSrgRunLog(key, logList, "Failed: Invalid Gapfilling Surrogate Sequence");
				continue;
			}

			if (g.equals("YES") && gapfill) {
				System.out.println(LS + "Gapfilling surrogate ratios for " + key + ": "
						+ (String) list.get(SURROGATE_INDEX));
				writeLogFile(LS + "Gapfilling surrogate ratios for " + key + ": " + (String) list.get(SURROGATE_INDEX)
						+ LS, runContinue);

				if (checkFile(srgFile = outDir + FS + key + FILL_TYPE + ".txt", "NONE", runContinue) && OW.equals("NO")) {
					// the output file exists and skip the surrogate
					writeLogFile(LS + "Surrogate ratio file exists for " + key + ": "
							+ (String) list.get(SURROGATE_INDEX) + LS, runContinue);
					putSrgRunLog(key, logList, "GAPFILLING Skipped");
					continue;
				}

				// create gapfilling txt file
				header = "# Gapfill surrogate ratios to generate surrogate -- " + key + "="
						+ (String) list.get(SURROGATE_INDEX);
				String gapfillFile = createGapfillFile(key, outDir + FS + tDir, header, (String) list
						.get(SURROGATE_INDEX), srgs);
				if (gapfillFile == null) {
					System.out.println("Creating GAPFILL TXT File Failed");
					putSrgRunLog(key, logList, "Creating GAPFILL TXT File Failed");
					continue;
				}

				if (hasSrgMerge) {
					COMMAND[2] = getControls("SRGMERGE EXECUTABLE") + "   " + gapfillFile;
					System.arraycopy(COMMAND, 0, cmd, 0, COMMAND.length);

					cmd[3] = gapfillFile;
					RunScripts rs = new RunScripts("SRGMERGE", COMMAND, env);
					runMessage = rs.run();
					rs = null; // free all memory used by rs
				} else {
					new SystemInfo().print();
					try {
						Gapfilling gf = new Gapfilling(gapfillFile);
						gf.doGapfill();
						runMessage = "SUCCESS RUNNING THE JAVA GAPFILL TOOL";
					} catch (Exception e) {
						System.out.println(e.getMessage());
						runMessage = "ERROR RUNNING THE JAVA GAPFILL TOOL";
					}
				}

				if (checkRunMessage(runMessage)) {
					putSrgRunLog(key, logList, "GAPFILLING Failed");
					System.out.println(key + " GAPFILLING Failed");
				} else {
					putSrgRunLog(key, logList, "GAPFILLING Success");
					System.out.println(key + " GAPFILLING Success");

					if (!addSrgHeaders(key, srgFile, GAPFILL_STATUS_INDEX)) {
						putSrgRunLog(key, logList, "GAPFILLING Headers Failed");
						System.out.println(LS + key + "   GAPFILLING Headers Failed" + LS);
					} else {
						addSrgDesc(key, (String) list.get(SURROGATE_INDEX), srgFile);
						writeSrgDesc();
					}
				}
			}// end of gapfilling
		}// end of j
	}

	public String writeSrgRunLog() {
		ArrayList list = new ArrayList();
		ArrayList rows = new ArrayList(); // all keys in the CSV file sequence

		if (runLog.isEmpty()) {
			return null;
		}

		StringBuffer runInfo = new StringBuffer();
		runInfo.append(LS + "\t\t" + "Surrogate Generation Summary" + LS + LS);
		rows = (ArrayList) generations.get(rowKey);
		for (int j = 0; j < rows.size(); j++) {
			String key = (String) rows.get(j);
			list = (ArrayList) generations.get(key);
			String generate = (String) list.get(GENERATE_INDEX);
			String g = generate.toUpperCase();
			if (g.equals("YES") && runLog.containsKey(key)) {
				runInfo.append(key + "\t");
				ArrayList runList = (ArrayList) runLog.get(key);
				for (int i = 0; i < runList.size() - 1; i++) {
					runInfo.append((String) runList.get(i) + "\t");
				}
				runInfo.append((String) runList.get(runList.size() - 1) + LS);
			}
		}
		return runInfo.toString(); // return srgRunlog information

	}

	/** -------------------------MAIN--------------------------------------* */
	public static void main(String[] arguments) {

		SurrogateTool sc = new SurrogateTool();

		if (arguments.length < 1) {
			System.out.println("Usge: java SrgTool controlFile");
			System.exit(SurrogateTool.runStop);
		}

		sc.getSystemInfo();

		sc.readControls(arguments[0]);
		sc.readSurrogates();
		sc.readShapefiles();
		sc.readGenerations();
		sc.readSurrogateCodes();

		sc.setMainVariables();
		sc.getGridPolyHeader();

		if (sc.run_create)
			sc.generateSurrogates();
		if (sc.run_merge)
			sc.mergeSurrogates();
		if (sc.run_gapfill)
			sc.gapfillSurrogates();
				
		sc.writeLogFile(sc.LS, SurrogateTool.runFinish); // exit the program run
		sc = null;
	}
}

/** -------------------------END of THE MAIN CLASS----------------------- */

// other classes
/**
 * Utility classes for generating surrogates 1. Checking a file existence. 2. Read header and check the variable items
 * 3. Parse comma-separated values (CSV), a common Windows file format.
 */

class CSV {
	protected String inFile;

	public static final char DEFAULT_SEP = ',';

	protected List list = new ArrayList(); // The fields in the readin line String

	protected Hashtable controls = new Hashtable(); // store items from the CSV file

	protected char fieldSep; // the separator char for this parser

	protected ArrayList listField; // Array of list elements to store field data

	/** Construct a CSV parser with default separator ","* */

	// Construct a CSV parser without input file and with the default separator ",".
	CSV() {
		fieldSep = DEFAULT_SEP;
	}

	// Construct a CSV parser with the default separator ",".
	CSV(String fileName) {
		fieldSep = DEFAULT_SEP;
		inFile = fileName;
	}

	// Construct a CSV parser with a given separator.
	CSV(String fileName, char sep) {
		inFile = fileName;
		fieldSep = sep;
	}

	/** check whether the CSV file exists* */
	void checkFile() {
		File f = new File(inFile);
		if (!f.exists()) {
			System.out.println("Error: Input File -- " + inFile + " Does Not Exist");
			System.exit(1);
		}

	}

	/*******************************************************************************************************************
	 * check header of the csv file -- does it contain all needed items. First item from items[] is the key items for
	 * the csv file
	 ******************************************************************************************************************/
	Hashtable readCSV(String[] items, int keyItems) {
		String line;
		/** string to read line in from the file* */
		int col = 0;
		int csvItems; // number of items in the CSV file
		Hashtable nameColumn = new Hashtable();
		String[] inItems;
		String rowKey = "ROWKEY"; // in hashtable key.
		ArrayList rows = new ArrayList(); // list to store all key sequence in the CSV file for future
											// reading,rowKey+row in the hashtable
		int headerCommas, lineCommas; // for comparing comma number in the header and in the rows
		String message; // error handling

		inItems = items;
		try {
			FileReader file = new FileReader(inFile);
			BufferedReader buff = new BufferedReader(file);

			/** read the header* */
			while ((line = buff.readLine()) != null) {
				if (line.matches("^\\s*$")) {
					continue;
				}
				break; // got header

			}

			/***********************************************************************************************************
			 * check whether the header contains all needed columns and read needed fields and column numbers into a
			 * hashtable
			 **********************************************************************************************************/
			list = parse(line.trim());
			csvItems = list.size(); // get the number of items in the csv file
			headerCommas = countcommas(line);
			// System.out.println(inFile + " header commas: "+headerCommas);
			for (int i = 0; i < inItems.length; i++) {
				if ((col = list.indexOf(inItems[i])) == -1) {
					// System.out.println("Error: Input File -- " + inFile + " Does Not have "+ inItems[i]+" field
					// needed");
					message = "Error: Input File -- " + inFile + " does not have " + inItems[i] + " field needed";
					listField = new ArrayList();
					listField.add(message);
					controls.put("CSV_ERROR", listField); // return to calling program due to the error
					return controls;
				}
				nameColumn.put(inItems[i], new Integer(col));
			}

			/** read in items into hashtable with ArrayList to return to main class* */

			int index = 0; // if keyItems = 2; all items into the list
			if (keyItems == 1) {
				index = 1; // keyitems = 1; items 2,3.... into the list
			}

			while ((line = buff.readLine()) != null) {

				// skip empty lines
				if (line.matches("^\\s*$")) {
					continue;
				}

				list = parse(line.trim());
				lineCommas = countcommas(line);
				// System.out.println("line commas: "+lineCommas);
				if (headerCommas != lineCommas) {
					// list.add(""); //handle csv table records with last element empty == undefined: missing after
					// parse
					message = "Error: Row has unmatched number of commas with header.  File -- " + inFile
							+ "  Line -- " + line;
					controls.clear();
					listField = new ArrayList();
					listField.add(message);
					controls.put("CSV_ERROR", listField); // return to calling program due to the error
					return controls;
				}

				// handle last column does not have data
				if (list.size() < csvItems) {
					list.add(""); // handle csv table records with last element empty == undefined: missing after
									// parse
				}

				listField = new ArrayList();
				for (int i = index; i < inItems.length; i++) {
					Integer temp_col = (Integer) nameColumn.get(inItems[i]);
					col = temp_col.intValue();
					// System.out.println("column "+col + " item "+inItems[i]+" "+ (String) list.get(col) );
					listField.add(list.get(col));
				}

				// get hashtable key
				Integer temp_col = (Integer) nameColumn.get(inItems[0]); // first key field value,
				col = temp_col.intValue();
				String key = (String) list.get(col); // first item is key
				if (keyItems == 2) {
					// first two items is key
					temp_col = (Integer) nameColumn.get(inItems[1]); // second key field value
					col = temp_col.intValue();
					String key2 = (String) list.get(col);
					key = key + "_" + key2;
				}

				// check duplicate table entries with the same key
				if (controls.containsKey(key)) {
					// System.out.println("Error: Input File -- " + inFile + " -- has duplicate entries for -- "+key);
					message = "Error: Input File -- " + inFile + " -- has duplicate entries for -- " + key;
					controls.clear();
					listField = new ArrayList();
					listField.add(message);
					controls.put("CSV_ERROR", listField); // return to calling program due to the error
					return controls;
				}
				controls.put(key, listField); // put each line into a hashtable
				rows.add(key);
			}
			buff.close();

		} catch (IOException e) {
			message = "Error in reading -- " + inFile + ": " + e.toString();
			controls.clear();
			listField = new ArrayList();
			listField.add(message);
			controls.put("CSV_ERROR", listField); // return to calling program due to the error
			return controls;
		}

		controls.put(rowKey, rows); // ROWKEY contains all keys in the CSV file sequence
		return controls;
	}

	/** count the number of commas separated culumns */
	protected int countcommas(String line) {

		int count = 0;
		int quote = 0;
		for (int i = 0; i < line.length(); i++) {
			if (quote == 0 && line.charAt(i) == ',') {
				count++;
			} else if (quote == 0 && line.charAt(i) == '"') {
				quote = 1;
			} else if (quote == 1 && line.charAt(i) == '"') {
				quote = 0;
			}
		}
		return count;
	}

	/**
	 * parse: break the input String into fields
	 * 
	 * @return java.util.Iterator containing each field from the original as a String, in order.
	 */
	protected List parse(String line) {
		StringBuffer sb = new StringBuffer();
		list.clear(); // recycle to initial state
		int i = 0;

		if (line.length() == 0) {
			list.add(line);
			return list;
		}

		do {
			sb.setLength(0);
			if (i < line.length() && line.charAt(i) == '"')
				i = advQuoted(line, sb, ++i); // skip quote
			else
				i = advPlain(line, sb, i);
			String sb_temp = sb.toString();
			list.add(sb_temp.trim());
			i++;
		} while (i < line.length());

		return list;
	}

	/** advQuoted: quoted field; return index of next separator */
	protected int advQuoted(String s, StringBuffer sb, int i) {
		int j;
		int len = s.length();
		for (j = i; j < len; j++) {
			if (s.charAt(j) == '"' && j + 1 < len) {
				if (s.charAt(j + 1) == '"') {
					j++; // skip escape char
				} else if (s.charAt(j + 1) == fieldSep) { // next delimeter
					j++; // skip end quotes
					break;
				}
			} else if (s.charAt(j) == '"' && j + 1 == len) { // end quotes at end of line
				break; // done
			}
			sb.append(s.charAt(j)); // regular character.
		}
		return j;
	}

	/** advPlain: unquoted field; return index of next separator */
	protected int advPlain(String s, StringBuffer sb, int i) {
		int j;

		j = s.indexOf(fieldSep, i); // look for separator
		if (j == -1) { // none found
			sb.append(s.substring(i));
			return s.length();
		}

		sb.append(s.substring(i, j));
		return j;

	}
}

/*--------class to handle Runtime class errors   for executive external exe files--------*/

class StreamGobbler extends Thread {
	InputStream is;

	String type;

	StringBuffer os;

	String LS = System.getProperty("line.separator");

	StreamGobbler(InputStream is, String type) {
		this(is, type, null);
	}

	StreamGobbler(InputStream is, String type, StringBuffer redirect) {
		this.is = is;
		this.type = type;
		this.os = redirect;
	}

	public void run() {
		try {

			InputStreamReader isr = new InputStreamReader(is);
			BufferedReader br = new BufferedReader(isr);
			String line = null;
			while ((line = br.readLine()) != null) {
				os.append(type + ">" + line + LS);
				// System.out.println(type + ">" + line);
			}
		} catch (IOException ioe) {
			os.append("ERROR>" + ioe.getMessage() + LS);
			ioe.printStackTrace();
		}
	}
}

/*----------------class to run a script file----------------------*/
class RunScripts {
	protected String[] cmd, env;

	protected String exeFile;

	StringBuffer mBuff = new StringBuffer(); // String to store message from the run

	String LS = System.getProperty("line.separator");

	// construct the class and check the file

	RunScripts(String exe, String[] command, String[] environments) {

		exeFile = exe;
		cmd = new String[command.length];
		env = new String[environments.length];
		System.arraycopy(command, 0, cmd, 0, command.length);
		System.arraycopy(environments, 0, env, 0, environments.length);
	}

	public String run() {
		int exitVal = -1; // run status: 0 --> normal , other --> failed

		try {
			Runtime rt = Runtime.getRuntime();
			Process proc = rt.exec(cmd, env);

			// any error message
			StreamGobbler errorGobbler = new StreamGobbler(proc.getErrorStream(), exeFile + "_ERROR", mBuff);

			// any output?
			StreamGobbler outputGobbler = new StreamGobbler(proc.getInputStream(), exeFile + "_OUTPUT", mBuff);

			// kick them off
			errorGobbler.start();
			outputGobbler.start();

			// any error???
			exitVal = proc.waitFor();

		} catch (Throwable t) {
			mBuff.append(exeFile + "_ERROR>" + t.getMessage() + LS);
		}

		if (exitVal == 0) {
			// System.out.println(exitVal+ " "+cmd[2]+" run succeed");
			mBuff.append("SUCCESS IN RUNNING THE EXECUTABLE: " + exeFile + LS);
		} else {
			// System.out.println(exitVal +" "+cmd[2]+ " run failed");
			mBuff.append("ERROR IN RUNNING THE EXECUTABLE: " + exeFile + LS);
		}

		System.runFinalization();
		System.gc();
		return mBuff.toString();
	}
}
