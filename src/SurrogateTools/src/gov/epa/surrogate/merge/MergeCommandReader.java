package gov.epa.surrogate.merge;

import gov.epa.surrogate.Command;
import gov.epa.surrogate.SurrogateFileInfo;
import gov.epa.surrogate.Surrogates;

import java.util.ArrayList;
import java.util.List;

public class MergeCommandReader {

	private Command command;

	private Surrogates surrogates;

	private List<Counties> countiesColl;

	private List<String> srgInfoHeader;

	public MergeCommandReader(Surrogates surrogates, Command command) {
		this.surrogates = surrogates;
		this.command = command;
		this.countiesColl = new ArrayList<Counties>();
		this.srgInfoHeader = new ArrayList<String>();
	}

	public void read() throws Exception {
		SurrogateFileInfo[] infos = command.getSurrogateInfos();
		for (int i = 0; i < infos.length; i++) {
			SurrogateFileReader reader = new SurrogateFileReader(surrogates,infos[i]);
			reader.read();
			countiesColl.add(reader.getCounties());
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
		srgInfoHeader.add("#SRGDESC=" + srgId + "," + srgName);
		//srgInfoHeader.add("#");
	} 

	public List<Counties> getCountiesColl() {
		return countiesColl;
	}

	public List<String> getSrgInfoHeader() {
		return srgInfoHeader;
	}
}
