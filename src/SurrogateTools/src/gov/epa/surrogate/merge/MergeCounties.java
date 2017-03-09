package gov.epa.surrogate.merge;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Set;
import java.util.TreeSet;

public class MergeCounties {

	private List<Counties> countiesColl;

	private Equation equation;

	private int outputSurrogateID;
	
	private String delimiter;

	public MergeCounties(List<Counties> countiesColl, Equation equation, int outputSurrogateID) {
		this.countiesColl = countiesColl;
		this.equation = equation;
		this.outputSurrogateID = outputSurrogateID;
		delimiter = "\t";
	}

	public Counties doMerge() throws Exception {
		Counties allCounties = new Counties();

		Set<Integer> codes = codes(countiesColl);
		int size = countiesColl.size();
		for (Iterator iter = codes.iterator(); iter.hasNext();) {
			County[] countiesArray = getCountiesArray(size, ((Integer) iter.next()).intValue());
		
			//replaceWithOutputSurrogateID(countiesArray, outputSurrogateID);
			
			allCounties.add(merge(countiesArray));			
		}
		return allCounties;
	}

//	private void replaceWithOutputSurrogateID(County[] counties, int outputSurrogateID) {
//		for (int i = 0; i < counties.length; i++) {
//			List<SurrogateRow> surrogates = counties[i].getSurrogates();
//			for (int j = 0; j < surrogates.size(); j++) {
//				surrogates.get(j).setSurrogateCode(outputSurrogateID);
//			}
//		}
//	}

	private County merge(County[] countiesArray) throws Exception {
		
		if (countiesArray.length == 1) {
			return mergeOneCounty(countiesArray[0]);
		}
		if (countiesArray.length > 2) {
			throw new Exception("Currently, merging is supported for a maximum of two surrogate files");
		}
		return mergeTwoCounties(countiesArray);
		
	}

	private County mergeOneCounty(County county) {
		List<SurrogateRow> rows = county.getSurrogates();
		Double sumValue = 0.0;
		for (int i = 0; i < rows.size(); i++) {
			SurrogateRow row = rows.get(i);
			int oldSCode = row.getSurrogateCode();
			sumValue = sumValue + row.getRatio();
			row.setSurrogateCode(outputSurrogateID);// replace with output surrogate id	 
			row.setComment( "Note: Only surrogate " + oldSCode + 
					" is available for this county" + delimiter + sumValue); // TODO: what should we put for comment when merging
		}
		return county;
	}

	private County mergeTwoCounties(County[] countiesArray) {
		County c1 = countiesArray[0];
		County c2 = countiesArray[1];
		
		List<SurrogateRow> rows1 = c1.getSurrogates();
		List<SurrogateRow> rows2 = c2.getSurrogates();		
		
		// find the indices for the arrays that are equal
		List<Integer> indices1 = new ArrayList<Integer>();
		List<Integer> indices2 = new ArrayList<Integer>();

		List<SurrogateRow> newRows = new ArrayList<SurrogateRow>();
		
		identifyingEqualRows(rows1, rows2, indices1, indices2);
//		System.out.println(rows1.size() + "  "+ rows2.size() + "  ");
//		System.out.println(indices1.size() + "  "+ indices2.size() + "  ");
		
		 mergeEqualCounties(rows1, rows2, indices1, indices2, newRows);		
			
		 mergeCountiesFromCounty1(rows1, indices1, newRows);		
		
		 mergeCountiesFromCounty2(rows2, indices2, newRows);		
		
		return newCounty(c1.getCountyCode(), newRows);
	}

	private County newCounty(int countyID, List<SurrogateRow> newRows) {
		County county = new County(countyID);
		// sorts the rows
		Set<SurrogateRow> newRowsSet = new TreeSet<SurrogateRow>(newRows);
		double sumValue = 0;
		for (Iterator iter = newRowsSet.iterator(); iter.hasNext();) {
			SurrogateRow row = (SurrogateRow) iter.next();
			sumValue = sumValue + row.getRatio();
			row.setComment(row.getComment() + "\t" + sumValue);
			county.addRow(row);
			//System.out.println("add: " + row.getSurrogateCode() + delimiter + row.getCountyCode());
		}
		return county;
	}

	private void mergeCountiesFromCounty2(List<SurrogateRow> rows2, List<Integer> indices2, 
			List<SurrogateRow> newRows) {
		for (int i = 0; i < rows2.size(); i++) {
			if (!indices2.contains(new Integer(i))) {
				SurrogateRow row = rows2.get(i);
				 
				double result = equation.evaluate(0.0, row.getRatio());
				SurrogateRow newRow = newSurrogateRow(row);
				newRow.setRatio(result);
				newRow.setComment(equation.getf1() + delimiter + "0.0" + delimiter +
						equation.getf2() + delimiter + row.getRatio() ); 
				newRows.add(newRow);
			}
		}
		 
	}

	private void mergeCountiesFromCounty1(List<SurrogateRow> rows1, List<Integer> indices1, 
			List<SurrogateRow> newRows) {
		 
		for (int i = 0; i < rows1.size(); i++) {
			if (!indices1.contains(new Integer(i))) {
				SurrogateRow row = rows1.get(i);
				double result = equation.evaluate(row.getRatio(), 0.0);
				SurrogateRow newRow = newSurrogateRow(row);
				newRow.setRatio(result);
				String comment = equation.getf1() + delimiter + row.getRatio()+ delimiter +
						equation.getf2() + delimiter + "0.0" + delimiter;
				
				newRow.setComment(comment); 
				newRows.add(newRow);
			}
		}
	}

	private void mergeEqualCounties(List<SurrogateRow> rows1, List<SurrogateRow> rows2, List<Integer> indices1,
			List<Integer> indices2, List<SurrogateRow> newRows) {
		 
		for (int i = 0; i < indices1.size(); i++) {
			SurrogateRow row = mergeRows(rows1.get(indices1.get(i)), rows2.get(indices2.get(i)));
			 
			row.setComment(row.getComment());
			newRows.add(row);
		}
	}

	private SurrogateRow mergeRows(SurrogateRow row1, SurrogateRow row2) {
		double result = equation.evaluate(row1.getRatio(), row2.getRatio());
		SurrogateRow newRow = newSurrogateRow(row1);
		newRow.setRatio(result);
		newRow.setComment(equation.getf1() + delimiter + row1.getRatio()+ delimiter +
				equation.getf2() + delimiter + row2.getRatio()+ delimiter); 
		return newRow;
	}

	private SurrogateRow newSurrogateRow(SurrogateRow row) {
		SurrogateRow newRow = new SurrogateRow();
		newRow.setSurrogateCode(outputSurrogateID);
		newRow.setCountyCode(row.getCountyCode());
	    newRow.setRow(row.getRow());
		newRow.setColumn(row.getColumn());

		return newRow;
	}

	private void identifyingEqualRows(List<SurrogateRow> rows1, List<SurrogateRow> rows2, List<Integer> indices1,
			List<Integer> indices2) {
		for (int i = 0; i < rows1.size(); i++) {
			SurrogateRow row1 = rows1.get(i);		
			
			for (int j = 0; j < rows2.size(); j++) {				
				if (row1.compareTo(rows2.get(j)) == 0) {			
					
					indices1.add(new Integer(i));
					indices2.add(new Integer(j));				
				}
			}
		}
	}

	private County[] getCountiesArray(int size, int countyCode) {
		List<County> counties = new ArrayList<County>();
		for (int j = 0; j < size; j++) {
			County county = null;
			try {
				county = countiesColl.get(j).getCounty(countyCode);
				counties.add(county);
			} catch (Exception e) {
				// FIXME: do not throw exception in this case
			}
		}
		return counties.toArray(new County[0]);
	}

	private Set<Integer> codes(List<Counties> countiesColl) {
		Set<Integer> codes = new TreeSet<Integer>();
		int size = countiesColl.size();
		for (int i = 0; i < size; i++) {
			codes.addAll(countiesColl.get(i).allCountyCodes());
		}
		return codes;
	}

}
