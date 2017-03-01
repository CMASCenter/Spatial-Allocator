package gov.epa.surrogate.gapfill;

public class PrimaryReplace implements Replace{

	public String replace(String line, int rowSrgID, int outputSurrogateID) {
		return line.replaceFirst(""+rowSrgID,""+outputSurrogateID);
	}


}
