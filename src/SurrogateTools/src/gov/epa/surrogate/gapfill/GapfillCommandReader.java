package gov.epa.surrogate.gapfill;

import gov.epa.surrogate.Command;
import gov.epa.surrogate.SurrogateFileInfo;
import gov.epa.surrogate.Surrogates;

import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.util.List;

public class GapfillCommandReader {

	private Command command;

	private Surrogates surrogates;

	private Counties counties;

	private List<String> comments;
	
	private List<String> srgInfoHeader;

	public GapfillCommandReader(Surrogates surrogates, Command command) {
		this.surrogates = surrogates;
		this.command = command;
		this.counties = new Counties();
		this.srgInfoHeader = new ArrayList<String>();
	}

	public void read() throws Exception {
		SurrogateFileInfo[] infos = command.getSurrogateInfos();
		int outputSurrogateID = surrogates.getSurrogateID(command.getOutputSurrogate());
		readPrimarySrg(infos[0],outputSurrogateID);
		
		for (int i = 1; i < infos.length; i++) {
			NonPrimarySurrogateFileReader reader = new NonPrimarySurrogateFileReader(surrogates, infos[i], counties, outputSurrogateID);
			reader.read();
		}
		doSrgInfoHeader();
	}

	private void doSrgInfoHeader() throws Exception {
		
		String outputSrgName = command.getOutputSurrogate();
		
		addSrgInfo(outputSrgName);
		SurrogateFileInfo[] gapfillInfos = command.getSurrogateInfos();
		for (int i = 0; i < gapfillInfos.length; i++) {
			addSrgInfo(gapfillInfos[i].getSurrogateName());
		}
	}

	private void addSrgInfo(String srgName) throws Exception {
		int srgId = surrogates.getSurrogateID(srgName);
		srgInfoHeader.add("#SRGDESC="+srgId+","+srgName);
		//srgInfoHeader.add("#");
	}

	private void readPrimarySrg(SurrogateFileInfo info, int outputSurrogateID) throws FileNotFoundException, Exception {
		PrimarySurrogateFileReader primarySrgeader = new PrimarySurrogateFileReader(surrogates, info, counties, outputSurrogateID);
		primarySrgeader.read();
		this.comments = primarySrgeader.getComments();
	}

	public Counties getCounties() {
		return counties;
	}

	public List<String> getPrimarySrgFileComments() {
		return comments;
	}

	public List<String> getSrgInfoHeader() {
		return srgInfoHeader;
	}
}
