package gov.epa.surrogate.gapfill;

import gov.epa.surrogate.Command;
import gov.epa.surrogate.Comment;
import gov.epa.surrogate.SurrogateFileInfo;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;

public class GridChecker {
	
	private Command[] commands;
	
	private String lastGrid;
	
	private String lastSurrogateFileName;

	public GridChecker(Command[] commands){
		this.commands = commands;
		lastGrid = null;
		lastSurrogateFileName = null;
	}
	
	public void verify() throws Exception{
		for (int i = 0; i < commands.length; i++) {
			Command command = commands[i];
			SurrogateFileInfo[] gapfillInfos = command.getSurrogateInfos();
			for (int j = 0; j < gapfillInfos.length; j++) {
				SurrogateFileInfo info = gapfillInfos[j];
				String grid = readGrid(info.getSurrogateFileName());
				verify(grid,info.getSurrogateFileName());
			}
		}
	}

	private void verify(String grid, String fileName) throws Exception {
		//TODO: check to see how to compare the grid
		if(lastGrid != null && !lastGrid.equalsIgnoreCase(grid)){
			throw new Exception("The grids in the following file do not match: '"+lastSurrogateFileName+ "', '"+ fileName+"'");
		}
		lastGrid = grid;
		lastSurrogateFileName = fileName;
	}

	private String readGrid(String surrogateFileName) throws Exception {
		BufferedReader reader = new BufferedReader(new FileReader(new File(surrogateFileName)));
		String line = null;
		Comment comment = new Comment();
		while((line = reader.readLine())!= null){
			line = line.trim();
			if(comment.isComment(line)){
				if(line.startsWith("#GRID") || line.startsWith("#POLYGON")){
					//return line.split("#GRID")[1].trim();
					return line;
				}
			}
		}
		throw new Exception("Could not find #GRID or #POLYGON tag in the file '"+surrogateFileName+"'");
	}
	
	public String gridHeader(){
		return lastGrid;
	}
}
