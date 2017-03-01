package gov.epa.surrogate.legacy;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.math.BigDecimal;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.List;
import java.util.Vector;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * This program is used to generate surrogate computation summary for the surrogate files listed in the SRGDESC.txt
 * file.
 * 
 * Usage: java SrgTool SRGDESC.txt Software requirement: java 2 Platform Standard Edition (J2SE) software Development
 * Date: February 2006 Developed by: Limei Ran, Carolina Environmental Program at UNC
 */

public class SrgSum {

	Hashtable srgDesc = new Hashtable(); // surrogate description information

	Hashtable counties = new Hashtable(); // hashtable for all counites

	Vector srgCodes = new Vector(); // hashtable for all surrogate codes

	public static final int runFinish = 0; // run finish without error

	public static final int runContinue = 1; // run continue without error

	public static final int runError = 2; // run continue with error

	public static final int runStop = 3; // run stop due to error

	String USER; // User who is running the program

	String CS, OS, FS, LS; // system related information: computer system,

	// operating type, file separator, line separator

	String GRIDPOLY_HEADER; // header for surrogates

	int srgMinItems; // minimum items in the surrogate files (items before QA

	// information)

	boolean gapFill = false; // indicator whether the surrogate is gapfilled

	// or not

	String outDir; // output directory

	String srgDescFile; // input SRGDESC file

	// set index
	public static final int REGION_INDEX = 0, SURROGATE_CODE_INDEX = 1, SURROGATE_INDEX = 2, SRGFILE_INDEX = 3;

	// method launch log file and to set system related variables
	public void getSystemInfo() {

		USER = System.getProperty("user.name");
		CS = System.getProperty("os.name").toLowerCase();
		OS = CS.toLowerCase();
		FS = System.getProperty("file.separator");
		LS = System.getProperty("line.separator");
		System.out.println("Your Operating system is:  " + OS + LS);
	}

	// method to check the existence of a file
	public boolean checkFile(String fileName, String delete) {

		String ow = delete.toUpperCase();
		String tfile = fileName;
		File tempfile = new File(tfile);

		// check the file, if it exists, delete it
		if (tempfile.exists() && ow.equals("YES")) {
			boolean success = tempfile.delete();
			if (!success) {
				System.out.println("Error: Deleting " + tfile + " file" + LS);
				return false;
			}
		}

		// the file has to be existing, otherwise error message
		if (!tempfile.exists() && ow.equals("NO")) {
			System.out.println("Error: File --- " + tfile + " -- Does Not Exist" + LS);
			return false;
		}

		// check the file existence, no message
		if (!tempfile.exists() && ow.equals("NONE")) {
			return false;
		}

		return true;
	}

	// method to check the existence of a directory
	public boolean checkDir(String dirName) {
		String tdir;

		tdir = dirName;
		File tempdir = new File(tdir);
		if (!tempdir.exists()) {
			boolean success = tempdir.mkdirs(); // create all directories
			if (!success) {
				System.out.println("Error: Making output directory -- " + dirName + LS);
				return false;
			}
		}
		return true;
	}

	private void readSrgDesc(String fileName) {
		String line; // one record from csv file
		List list = new ArrayList();
		int ITEMS = 4; // four items in SRGDESC.txt file
		System.out.println("Reading file -- " + fileName);

		srgDescFile = fileName;
		File cfile = new File(fileName);
		outDir = cfile.getParent();
		if (outDir == null) {
			// only specify name, so use the current directory for output
			// summary file
			outDir = "."; // current directory
		}

		System.out.println("Output Directory is:  " + outDir);

		CSV csv = new CSV(); // for parsing input csv file line

		// check SRGDESC file existence
		if (!checkFile(fileName, "NONE")) {
			System.out.println("Error: SRGDESC.txt does not exist -- " + fileName);
			System.exit(runStop);
		}

		try {
			FileReader file = new FileReader(fileName);
			BufferedReader buff = new BufferedReader(file);

			while ((line = buff.readLine()) != null) {
				// System.out.println("SRGDESC = "+line+LS);
				// get rid of headers and empty lines
				if (line.matches("^\\#.*")) {
					GRIDPOLY_HEADER = line + LS;
					Pattern p = Pattern.compile("GRID");
					Matcher m = p.matcher(GRIDPOLY_HEADER);
					if (m.find()) {// assume it is grid or egrid output
						srgMinItems = 5;
					} else {// assume it is polygon output
						srgMinItems = 4;
					}
					System.out.println("srgMinItems =  " + Integer.toString(srgMinItems) + LS);
					continue;
				}

				if (line.matches("^\\s*$")) {
					continue;
				}

				list = csv.parse(line.trim());
				// check total items in each line
				if (list.size() != ITEMS) {
					System.out.println("Warning: Wrong format line in " + fileName + ": " + line + LS);
					System.exit(runStop);
				}

				String region = (String) list.get(REGION_INDEX);
				String code = (String) list.get(SURROGATE_CODE_INDEX);
				String srgName = (String) list.get(SURROGATE_INDEX);
				String srgFile = (String) list.get(SRGFILE_INDEX);

				String key = region + "_" + code;
				String newline = region + "|" + code + "|" + srgName + "|" + srgFile;
				System.out.println("SRGDESC: " + newline + LS);
				srgDesc.put(key, newline);
				srgCodes.add(code);
			}
			buff.close();
		} catch (IOException e) {
			System.out.println("Error: In reading existing OUTPUT SRGDESC FILE --" + e.toString() + LS);
		}

		System.out.println("Finished reading the SRGDESC file" + LS);
	}

	// read the surrogate files for summary information
	private void readSrgFiles() {
		String line; // one record from csv file
		List list = new ArrayList();
		int COUNTY_INDEX = 1; // county ID position in srg file line
		int RATIO_INDEX = srgMinItems - 1; // surrogate ratio position in srg
		// file line
		int GAPFILL_CODE_INDEX = 1; // index for gapfilled code
		String fillCode;
		Hashtable srgs;
		String total;
		String county_s;
		Integer county;
		String ratio_s;
		double ratio;

		System.out.println("Reading surrogate files...");

		// return if no new surrogate files created
		if (srgDesc.isEmpty()) {
			System.out.println("No Surrogate files in the SRGDESC file.");
			return;
		}

		CSV csv = new CSV("tempfile", '\t'); // for parsing input surrogate
		// file line seperated by \t

		// get a surrogate file and read its information
		// sort hashtable
		Vector v = new Vector(srgDesc.keySet());
		Collections.sort(v);
		Iterator it = v.iterator();
		while (it.hasNext()) {
			String key = (String) it.next();
			line = (String) srgDesc.get(key);

			// get surrogate name and file
			String[] rcnf = line.split("\\|"); // split partial function
			String code = rcnf[SURROGATE_CODE_INDEX];
			String srgFile = rcnf[SRGFILE_INDEX];

			System.out.println("Reading " + srgFile);
			Pattern p = Pattern.compile("NOFILL");
			Matcher m = p.matcher(srgFile);
			if (m.find()) {// it is not gapfilled
				gapFill = false;
			} else { // it is gapfilled
				gapFill = true;
			}
			fillCode = code; // initialize

			if (!checkFile(srgFile, "NONE")) {
				System.out.println("Error: Surrogate File does not exist -- " + srgFile);
				System.exit(runStop);
			}

			try {
				FileReader file = new FileReader(srgFile);
				BufferedReader buff = new BufferedReader(file);

				Integer pre_county = Integer.valueOf(-99999);
				double totalc_ratio = 0.0;
				boolean finishReading = false;
				while (!finishReading) {
					line = buff.readLine();
					if (line != null) {
						// get rid of headers and empty lines
						if (line.matches("^\\#.*") || line.matches("^\\s*$")) {
							continue;
						}

						list = csv.parse(line.trim());
						// check total items in each line
						if (list.size() < srgMinItems) {
							System.out.println("Warning: Wrong format line in " + srgFile + ": " + line + LS);
							System.exit(runStop);
						}

						county_s = (String) list.get(COUNTY_INDEX);
						county = Integer.valueOf(county_s.trim());
						ratio_s = (String) list.get(RATIO_INDEX);
						ratio = Double.valueOf(ratio_s.trim()).doubleValue();
						// System.out.println(code + " "+county+ " "+ratio);
					} else {
						finishReading = true;
						county = Integer.valueOf(-88888); // temp county for ending
						ratio = 0.0;
					}

					if (!pre_county.equals(county) && !pre_county.equals(Integer.valueOf(-99999))) {
						BigDecimal bd = new BigDecimal(totalc_ratio);
						double total_5d = bd.setScale(5, BigDecimal.ROUND_HALF_UP).doubleValue();

						if (code.equals(fillCode)) { // not gapfilled line
							if (Math.abs(1.0 - totalc_ratio) > 0.00001) {
								total = "NOT1.0" + ":" + Double.toString(total_5d);
							} else {
								total = "";
							}
						} else { // gapfilled line
							if (Math.abs(1.0 - totalc_ratio) > 0.00001) {
								total = "GF:" + fillCode + ":NOT1.0:" + Double.toString(total_5d);
							} else {
								total = "GF:" + fillCode;
							}
						}

						if (!counties.containsKey(pre_county)) {
							srgs = new Hashtable();
						} else {
							srgs = (Hashtable) counties.get(pre_county);
							counties.remove(pre_county);
						}
						srgs.put(code, total);
						counties.put(pre_county, srgs);
						totalc_ratio = 0.0;
					}

					// for not the end of file reading
					if (line != null) {
						totalc_ratio += ratio;
						pre_county = county;
						if (gapFill) {
							Pattern pp = Pattern.compile("GF:");
							Matcher mm = pp.matcher(line);
							if (mm.find()) {// it is gapfilled
								String[] fc = line.split("GF:"); // split
								// partial
								// function
								String fillCode_s = fc[GAPFILL_CODE_INDEX];
								fillCode = fillCode_s.trim();
								// System.out.println("GAP county = "+county+"
								// code = "+code+" fillCode = "+fillCode);
							} else { // it is not gapfilled
								fillCode = code;
							}
						} // end of gapFill
					} // end of line!=null

				} // end of while
				buff.close();
			} catch (IOException e) {
				System.out.println("Error: In reading existing OUTPUT SRGDESC FILE --" + e.toString() + LS);
			}
		} // end of read each srg file
		System.out.println("Finished reading each surrogate information." + LS);
	}

	// write srg summary information into a file
	private void writeSrgSum() {
		String outFile = outDir + FS + "srg_sum.csv";
		String total;

		System.out.println("The output surrogate summary file is: " + outFile + LS);
		if (checkFile(outFile, "NONE")) {
			System.out.println("The output file exists: " + outFile + LS);
			System.exit(runStop);
		}

		// return if no county has surroagte
		if (counties.isEmpty()) {
			System.out.println("No County has surrogate data.");
			return;
		}

		try {
			// open output file
			FileWriter fw = new FileWriter(outFile); // overwrite the
			// existing file whether
			// or not it is deleted
			BufferedWriter out = new BufferedWriter(fw);

			// write grid headers
			out.write(GRIDPOLY_HEADER);
			out.write(srgDescFile + LS);
			// sort surrogate code vector
			Collections.sort(srgCodes);
			out.write("COUNTY");
			for (int i = 0; i < srgCodes.size(); i++) {
				String code = (String) srgCodes.elementAt(i);
				out.write("," + code);
			}
			out.write(LS);

			// output surrogate summary data based on county from the sorted
			// hashtable
			Vector v = new Vector(counties.keySet());
			Collections.sort(v);
			Iterator it = v.iterator();
			while (it.hasNext()) {
				Integer key = (Integer) it.next();
				Hashtable srgs = (Hashtable) counties.get(key);
				out.write(key.toString());
				for (int i = 0; i < srgCodes.size(); i++) {
					String code = (String) srgCodes.elementAt(i);
					if (srgs.containsKey(code)) {
						total = (String) srgs.get(code);
					} else {
						total = "NODATA";
					}
					out.write("," + total);
				}
				out.write(LS);
			}
			out.close();
		} catch (IOException e) {
			System.out.println("Error: In writting OUTPUT SUMMARY FILE --" + e.toString() + LS);
		}

		counties = null;
		srgCodes = null;
		srgDesc = null;
		System.out.println("Finished writting summary file.");
	}

	public void execute(String srgDescFile) {
		getSystemInfo();
		readSrgDesc(srgDescFile);
		readSrgFiles();
		writeSrgSum();
	}

	/** -------------------------MAIN--------------------------------------* */
	public static void main(String[] arguments) {
		SrgSum ss = new SrgSum();

		if (arguments.length < 1) {
			System.out.println("Usge: java SrgSum SRGDESC.txt");
			System.exit(1);
		}
		ss.execute(arguments[0]);
	}
}

/** -------------------------END of THE MAIN CLASS----------------------- */

// other classes
/**
 * Utility classes for generating surrogates 1. Checking a file existence. 2. Read header and check the variable items
 * 3. Parse comma-separated values (CSV), a common Windows file format.
 */

