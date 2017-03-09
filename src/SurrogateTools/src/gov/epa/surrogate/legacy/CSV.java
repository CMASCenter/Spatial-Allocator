package gov.epa.surrogate.legacy;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Hashtable;
import java.util.List;

public class CSV {
	
	protected String inFile;

	public static final char DEFAULT_SEP = ',';

	protected List list = new ArrayList(); // The fields in the readin line

	// String

	protected Hashtable controls = new Hashtable(); // store items from the CSV

	// file

	protected char fieldSep; // the separator char for this parser

	protected ArrayList listField; // Array of list elements to store field

	// data

	/** Construct a CSV parser with default separator ","* */

	// Construct a CSV parser without input file and with the default separator
	// ",".
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

	/***************************************************************************
	 * check header of the csv file -- does it contain all needed items. First
	 * item from items[] is the key items for the csv file
	 **************************************************************************/
	Hashtable readCSV(String[] items, int keyItems) {
		String line;
		/** string to read line in from the file* */
		int col = 0;
		int csvItems; // number of items in the CSV file
		Hashtable nameColumn = new Hashtable();
		String[] inItems;
		String rowKey = "ROWKEY"; // in hashtable key.
		ArrayList rows = new ArrayList(); // list to store all key sequence in
		// the CSV file for future
		// reading,rowKey+row in the
		// hashtable
		int headerCommas, lineCommas; // for comparing comma number in the
		// header and in the rows
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

			/*******************************************************************
			 * check whether the header contains all needed columns and read
			 * needed fields and column numbers into a hashtable
			 ******************************************************************/
			list = parse(line.trim());
			csvItems = list.size(); // get the number of items in the csv file
			headerCommas = countcommas(line);
			// System.out.println(inFile + " header commas: "+headerCommas);
			for (int i = 0; i < inItems.length; i++) {
				if ((col = list.indexOf(inItems[i])) == -1) {
					// System.out.println("Error: Input File -- " + inFile + "
					// Does Not have "+ inItems[i]+" field needed");
					message = "Error: Input File -- " + inFile + " Does Not have " + inItems[i] + " field needed";
					listField = new ArrayList();
					listField.add(message);
					controls.put("CSV_ERROR", listField); // return to calling
					// program due to
					// the error
					return controls;
				}
				nameColumn.put(inItems[i], new Integer(col));
			}

			/**
			 * read in items into hashtable with ArrayList to return to main
			 * class*
			 */

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
					// list.add(""); //handle csv table records with last
					// element empty == undefined: missing after parse
					message = "Error: Row has unmatched number of commas with header.  File -- " + inFile
							+ "  Line -- " + line;
					controls.clear();
					listField = new ArrayList();
					listField.add(message);
					controls.put("CSV_ERROR", listField); // return to calling
					// program due to
					// the error
					return controls;
				}

				// handle last column does not have data
				if (list.size() < csvItems) {
					list.add(""); // handle csv table records with last
					// element empty == undefined: missing after
					// parse
				}

				listField = new ArrayList();
				for (int i = index; i < inItems.length; i++) {
					Integer temp_col = (Integer) nameColumn.get(inItems[i]);
					col = temp_col.intValue();
					// System.out.println("column "+col + " item "+inItems[i]+"
					// "+ (String) list.get(col) );
					listField.add(list.get(col));
				}

				// get hashtable key
				Integer temp_col = (Integer) nameColumn.get(inItems[0]); // first
				// key
				// field
				// value,
				col = temp_col.intValue();
				String key = (String) list.get(col); // first item is key
				if (keyItems == 2) {
					// first two items is key
					temp_col = (Integer) nameColumn.get(inItems[1]); // second
					// key
					// field
					// value
					col = temp_col.intValue();
					String key2 = (String) list.get(col);
					key = key + "_" + key2;
				}

				// check duplicate table entries with the same key
				if (controls.containsKey(key)) {
					// System.out.println("Error: Input File -- " + inFile + "
					// -- has duplicate entries for -- "+key);
					message = "Error: Input File -- " + inFile + " -- has duplicate entries for -- " + key;
					controls.clear();
					listField = new ArrayList();
					listField.add(message);
					controls.put("CSV_ERROR", listField); // return to calling
					// program due to
					// the error
					return controls;
				}
				controls.put(key, listField); // put each line into a
				// hashtable
				rows.add(key);
			}
			buff.close();

		} catch (IOException e) {
			message = "Error in reading -- " + inFile + ": " + e.toString();
			controls.clear();
			listField = new ArrayList();
			listField.add(message);
			controls.put("CSV_ERROR", listField); // return to calling program
			// due to the error
			return controls;
		}

		controls.put(rowKey, rows); // ROWKEY contains all keys in the CSV file
		// sequence
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
	 * @return java.util.Iterator containing each field from the original as a
	 *         String, in order.
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
			} else if (s.charAt(j) == '"' && j + 1 == len) { // end quotes at
				// end of line
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
