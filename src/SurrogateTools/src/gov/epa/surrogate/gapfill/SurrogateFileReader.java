package gov.epa.surrogate.gapfill;

import gov.epa.surrogate.Comment;
import gov.epa.surrogate.SrgCodeLineParser;
import gov.epa.surrogate.SurrogateFileInfo;
import gov.epa.surrogate.Surrogates;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.util.ArrayList;
import java.util.List;

public class SurrogateFileReader {

	private Surrogates surroates;

	private BufferedReader reader;

	private String surrogateName;

	private List<String> comments;

	private SrgCodeLineParser srgCodeLineParser;

	private String delimiter;

	private boolean foundID;

	private int surrogateID;

	private Counties counties;

	private int previousCountyID;

	private int ouputSurrogateID;

	// TODO: try to refactor the code: GapfillInfo and SurrogateDescription
	public SurrogateFileReader(Surrogates surrogates, SurrogateFileInfo gapfillInfo, Counties counties,
			int outputSurrogateID) throws FileNotFoundException {
		this.surroates = surrogates;
		this.reader = new BufferedReader(new FileReader(new File(gapfillInfo.getSurrogateFileName())));
		this.surrogateName = gapfillInfo.getSurrogateName();
		this.comments = new ArrayList<String>();
		this.srgCodeLineParser = new SrgCodeLineParser(surrogates);
		this.delimiter = "\t";

		this.foundID = false;
		this.surrogateID = -1;
		this.ouputSurrogateID = outputSurrogateID;
		this.counties = counties;
		this.previousCountyID = -1;
	}

	public void read(Replace replace) throws Exception {
		String line = null;
		Comment comment = new Comment();
		County county = null;
		while ((line = reader.readLine()) != null) {
			line = line.trim();
			if (comment.isComment(line)) {
				processComment(line);
			} else {
				county = processData(line, county, replace);
			}
		}
		addLastCounty(county);
	}

	private void processComment(String line) throws Exception {
		if (!line.startsWith("#GRID") && !line.startsWith("#POLYGON")) {
			comments.add(line);
		}
		srgCodeLineParser.parse(line);
	}

	private County processData(String line, County county, Replace replace) throws Exception {
		String[] tokens = parse(line);
		int rowSrgID = Integer.parseInt(tokens[0].trim());
		int countyID = Integer.parseInt(tokens[1].trim());
		if (rowSrgID == surrogateID()) {
			line = replace.replace(line, rowSrgID, ouputSurrogateID);
			county = addDataRow(line, county, countyID);
		}
		return county;
	}

	private County addDataRow(String line, County county, int countyID) throws Exception {
		if (isNewCounty(countyID)) {
			addCounty(county);// add the last read county to the collection before creating a new county
			county = new County(countyID, surrogateID());
		}
		county.addRow(line);
		return county;
	}

	private void addCounty(County county) {
		if (county == null) {
			return;
		}

		int countyID = county.getCountyID();
		if (!counties.exist(countyID)) {
			counties.add(countyID, county);
		}
	}

	private void addLastCounty(County county) {
		addCounty(county);
	}

	private int surrogateID() throws Exception {
		if (!foundID) {
			surrogateID = surroates.getSurrogateID(surrogateName);
			foundID = true;
		}
		return surrogateID;
	}

	private String[] parse(String line) {
		String[] tokens = line.split(delimiter);
		return tokens;
	}

	private boolean isNewCounty(int countyID) {
		boolean newCounty = true;

		if (previousCountyID == -1 || previousCountyID != countyID) {
			previousCountyID = countyID;
		} else if (previousCountyID == countyID) {
			newCounty = false;
		}
		return newCounty;
	}

	public List<String> getComments() {
		return comments;
	}
}
