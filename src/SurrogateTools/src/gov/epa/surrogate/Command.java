package gov.epa.surrogate;

public interface Command {
	
	SurrogateFileInfo[] getSurrogateInfos();
	
	String getOutputSurrogate();

}
