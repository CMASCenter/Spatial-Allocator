package gov.epa.surrogate.normalize;

import gov.epa.surrogate.Comment;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.util.ArrayList;
import java.util.List;

public class ExcludeCountiesFileReader {

	private BufferedReader reader;

	private String fileName;

	public ExcludeCountiesFileReader(String fileName) throws FileNotFoundException {
		this.fileName = fileName;
		reader = new BufferedReader(new FileReader(new File(fileName)));
	}

	public int[] read() throws Exception {
		List<Integer> counties = new ArrayList<Integer>();
		String line = null;
		Comment comment = new Comment();
		while ((line = reader.readLine()) != null) {
			if (!comment.isComment(line)) {
				counties.add(value(line.trim()));
			}
		}
		reader.close();
		return intValues(counties);
	}

	private int[] intValues(List counties) {
		int size = counties.size();
		int[] countyCodes = new int[size];
		for (int i = 0; i < size; i++) {
			countyCodes[i] = ((Integer) counties.get(i)).intValue();
		}
		return countyCodes;
	}

	private Integer value(String line) throws Exception {
		try {
			return Integer.valueOf(line);
		} catch (NumberFormatException e) {
			throw new Exception("Error reading the file '" + fileName + "'; " + line + " is not a county id");
		}
	}
}