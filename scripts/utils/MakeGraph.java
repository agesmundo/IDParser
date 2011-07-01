import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.io.Reader;
import java.nio.charset.Charset;
import java.text.DecimalFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class MakeGraph{

	public static void main(String args[]){
		String usage = "Usage:\n\tMakeGraph <log_file> <out_file> [file_to_compare]";
		if(args.length<2){
			System.out.println(usage);
			return;
		}
		String logFile=args[0];
		String outFile=args[1];
		System.out.println("MakeGraph");

		List <Graph> compareGraphs=new ArrayList<Graph>();
		for(int i=2;i<args.length;i++){
			compareGraphs.add(new Graph(args[i]));
		}

		Graph graph=new Graph(logFile);
		graph.printGnuPlot(outFile);
		graph.printLatex(outFile,compareGraphs);
	}

	public static class Graph{
		private List<List<String>> metrics; //[column][line]
		private List<String> fieldsNames;
		private String name;

		public Graph(String logFile){
			name=logFile.replaceFirst("^(.+/)*", "");
			System.out.println("\nReading file: "+logFile);
			
			String columnsName[]={"Round","Synt-LAS","Srl-P,R","Overall"};
			fieldsNames=Arrays.asList(columnsName);
			metrics=new ArrayList<List<String>>();
			for(int i =0; i<fieldsNames.size();i++){
				metrics.add(new ArrayList<String>());
			}

			List <String> lines = getListString(logFile);
			int count=1;
			for(String line: lines){
				if (line.contains("Testing accuracy")){
					System.out.println(line = line.replaceAll(":", "").replaceAll(";", " ").replaceAll("%", "").trim());
					String[] tokens = line.split(" ");
					if(Integer.parseInt(tokens[0])!=count){
						System.err.println(tokens[0]+" != "+count);
					}
					String [] ss = {tokens[0],tokens[4],tokens[9],tokens[12]};
					this.addLine(ss);
					count++;
				}	
			}
		}

		private void addLine(String[] line){

			for(int i =0 ; i < line.length;i++){
				metrics.get(i).add(line[i]);
			}
		}

		public void printGnuPlot (String fileName) {
			print(fileName+"_data",getTable());
			print(fileName+"_script",getGnuScript(fileName));
		}

		public  void printLatex (String fileName,List <Graph> compareFiles) {
			print(fileName+".tex",getLatex(fileName, compareFiles));
		}

		void print (String fileName,String content) {
			try{
				PrintWriter out = new PrintWriter (new FileOutputStream(fileName));//write out file
				out.write(content);
				out.close();
			}catch(FileNotFoundException e){
				System.out.println(e);
			}
		}

		String getLatex(String fileName,List <Graph> compareGraphs) {
			LatexFactory lf= new LatexFactory();
			lf.addHead(fileName.replaceFirst("^(.*/)*", ""));
			lf.addPicture(fileName+"_plot.eps");
			lf.addHead("Best: "+getName());

			List<List<String>> entries=new ArrayList<List<String>>();
			entries.add(fieldsNames);
			List<String> values=new ArrayList<String>();
			for(String value :getLine(getHighestID())){
				if(value.contains(".")){
					value=roundTwoDecimals(Double.parseDouble(value))+"\\%";
				}
				values.add(value);
			}
			entries.add(values);
			lf.addTable(entries);

			for(int i=0; i<compareGraphs.size(); i++){
				lf.addHead("Comparison with: "+compareGraphs.get(i).getName());
				entries=new ArrayList<List<String>>();
				List<String> column=new ArrayList<String>();
				column.add("");
				column.addAll(fieldsNames.subList(1, fieldsNames.size()));
				entries.add(column);
				entries.addAll(	compareBestsWith(compareGraphs.get(i)));
				lf.addTable(entries);
			}
			return lf.toString();
		}

		String getName(){
			return name;
		}

		String getGnuScript(String fileName) {
			String rtn= "set term postscript eps	" +
			"\nset output \""+fileName+"_plot.eps\" " +
			"\nset yr [50:95]" +
			"\nset key "+(0.9 * metrics.get(0).size())+",55 box " +
			"\nset xlabel \"Iterations on traing set\"" +
			"\nset ylabel \"(%)\"" +
			"\nplot ";
			for(int i=1;i<metrics.size();i++){
				rtn+="\""+fileName+"_data\" using 1:"+(i+1)+" title '"+fieldsNames.get(i)+"' with lines "+(i)+"";
				if(i!=metrics.size()-1)rtn+=", \\\n";
			}
			return rtn;
		}

		String getTable() {
			String ret="#";

			//print names
			for(int j=0;j<fieldsNames.size();j++){
				ret+=fieldsNames.get(j)+" ";
			}

			//print metrics for each round
			for(int i=0;i<metrics.get(0).size();i++){
				ret+="\n";
				for(int j=0;j<metrics.size();j++){
					ret+=metrics.get(j).get(i)+" ";
				}
			}
			return ret;
		}

		int getHighestID(){
			return getHighestID(fieldsNames.size()-1);
		}

		int getHighestID(int columnId){
			int lineId=0;
			Double bestVal=Double.MIN_VALUE;
			for(int i=0;i<metrics.get(columnId).size();i++){
				if(Double.parseDouble(metrics.get(columnId).get(i))>bestVal){
					bestVal=Double.parseDouble(metrics.get(columnId).get(i));
					lineId=i;
				}
			}

			return lineId; 
		}

		List<List <String>> compareBestsWith(Graph comp){
			List<List <String>> rtn = new ArrayList<List<String>>();
			List <String> prevBest= comp.getLine(comp.getHighestID()).subList(1, fieldsNames.size());
			List <String> currentBest= getLine(getHighestID()).subList(1, fieldsNames.size());

			List <String> column = new ArrayList<String>();
			column.add("Scores");
			for(int i=0;i<prevBest.size();i++){
				column.add(roundTwoDecimals(
						Double.parseDouble(prevBest.get(i))
						)+"\\%");
			}
			rtn.add(column);

			column = new ArrayList<String>();
			column.add("Difference");
			for(int i=0;i<prevBest.size();i++){
				column.add(roundTwoDecimals(
						(Double.parseDouble(currentBest.get(i))-Double.parseDouble(prevBest.get(i)))
						)+"\\%");
			}
			rtn.add(column);

			column = new ArrayList<String>();
			column.add("Error Riduction");
			for(int i=0;i<prevBest.size();i++){
				column.add(
						compER(Double.parseDouble(currentBest.get(i)),Double.parseDouble(prevBest.get(i)))+"\\%"
				);
			}
			rtn.add(column);

			return rtn;
		}

		double compER(double curr, double prev){
			double res = (curr-prev)*100/(prev);
			return roundTwoDecimals(res);
		}
		
		double roundTwoDecimals(double d) {
			DecimalFormat twoDForm = new DecimalFormat("#.##");
			return Double.valueOf(twoDForm.format(d));
		}

		List<String> getLine(int lineId){
			List<String> rtn = new ArrayList<String>();
			for(int i =0;i<metrics.size();i++){
				rtn.add(metrics.get(i).get(lineId));
			}
			return rtn;
		}

		String printLine(int lineId){
			String ret="";
			for(int j=0;j<metrics.size();j++){
				ret+=fieldsNames.get(j)+":\t"+metrics.get(j).get(lineId)+"\n";
			}
			return ret;
		}

		static List<String> getListString(String fileName) {
			Charset defaultCharset= Charset.forName("UTF-8");
			Reader reader=null;
			try{
				reader=new BufferedReader(new InputStreamReader(new FileInputStream(fileName),defaultCharset));
			}catch(FileNotFoundException e){
				e.printStackTrace();
			}
			List <String> lines=new ArrayList<String>();
			try{
				BufferedReader in = new BufferedReader ( reader );
				String line=in.readLine();
				while(line!=null){
					lines.add(line);
					line=in.readLine();
				}
				in.close();
			} catch (IOException e) {
				System.err.println(e.toString());
			}
			return lines;
		}

	}

	public static class LatexFactory{
		StringBuffer content=new StringBuffer();

		public LatexFactory(){
			content.append(
					"\\documentclass{article}\n"+
					"\\usepackage[latin1]{inputenc}\n"+
					"\\usepackage{graphicx}\n"+
					"\\begin{document}\n"
			);
		}

		public void addPicture(String name){
			content.append(
					"\n"+
					"\\begin{figure}[htp]\n"+
					"\\includegraphics{"+name+"}\n"+
					"\\end{figure}\n"
			);		
		}

		public void addVerbatim(String text){
			content.append(
					"\n"+
					"\\begin{verbatim}\n"+
					""+text+"\n"+
					"\\end{verbatim}\n"
			);		
		}

		public void addHead(String head){
			content.append(
					"\n\n"+
					"{\\Large "+head+"}\\\\\n\n"
			);
		}

		public void addTable(List <List<String>>entries){//[line][column]
			content.append("\\begin{tabular}{");
			for(int i=0;i<entries.size();i++){
				content.append("l");
			}
			content.append("}\n");	
			for(int i=0;i<entries.get(0).size();i++){
				for(int j=0;j<entries.size();j++){
					content.append(entries.get(j).get(i)+"\t");
					if(j==entries.size()-1){
						content.append("\\\\\n");	
					}
					else{
						content.append("&\t");
					}
				}
			}
			content.append("\\end{tabular}\n\\\\\\\\\n");
		}

		public String toString(){
			content.append("\n\\end{document}");
			return content.toString();
		}
	}
}
